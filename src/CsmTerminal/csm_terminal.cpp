#include "../CsmTerminal/csm_terminal.hpp"

#include <csignal>
#include <cstdlib>
#include <ctime>
#include <future>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "../Logger/logger.hpp"

namespace csm_cmd
{

csm_terminal* csm_terminal::active_instance_ = nullptr;

namespace
{

std::string homeDirectory()
{
  const char* home = std::getenv("HOME");
  if (home != nullptr)
  {
    return std::string(home);
  }
  return std::string(".");
}

}  // namespace

csm_terminal::csm_terminal(std::chrono::milliseconds command_timeout)
  : timeout_(command_timeout)
{
  history_path_ = homeDirectory() + "/.csm_cmd_history";

  Logger::instance().configure(homeDirectory() + "/.csm_cmd.log");
  Logger::instance().info("csm_terminal initialized");

  repl_.install_window_change_handler();
  repl_.set_max_history_size(static_cast<int>(kMaxHistoryLines));
  repl_.set_completion_count_cutoff(32);
  repl_.set_word_break_characters(" \t");

  setupBuiltins();
  setupCompletion();
  setupHighlighter();
  loadHistory();

  installSignalHandler(this);
}

csm_terminal::~csm_terminal()
{
  saveHistory();
  if (active_instance_ == this)
  {
    active_instance_ = nullptr;
  }
  Logger::instance().info("csm_terminal shutting down");
}

void csm_terminal::installSignalHandler(csm_terminal* self)
{
  active_instance_ = self;
  std::signal(SIGINT, &csm_terminal::signalHandler);
}

void csm_terminal::signalHandler(int signum)
{
  (void)signum;
  if (active_instance_ != nullptr)
  {
    Logger::instance().warn("received SIGINT, shutting down gracefully");
    active_instance_->stop();
  }
}

std::string csm_terminal::escapeOutput(const std::string& text)
{
  std::string escaped;
  escaped.reserve(text.size());

  for (const unsigned char c : text)
  {
    if (c == 0x1b)
    {
      escaped += "\\x1b";
    }
    else if (c == '\r')
    {
      escaped += "\\r";
    }
    else if (c < 0x20 && c != '\n' && c != '\t')
    {
      std::ostringstream oss;
      oss << "\\x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(c);
      escaped += oss.str();
    }
    else
    {
      escaped.push_back(static_cast<char>(c));
    }
  }

  return escaped;
}

void csm_terminal::registerCommand(const std::string& name, CommandRegistry::CommandHandler handler, const std::string& description)
{
  registry_.registerCommand(name, std::move(handler), description);
  Logger::instance().info("registered command: " + name);
}

void csm_terminal::registerAlias(const std::string& alias, const std::string& target)
{
  registry_.registerAlias(alias, target);
  Logger::instance().info("registered alias: " + alias + " -> " + target);
}

void csm_terminal::setupCompletion()
{
  repl_.set_completion_callback([this](const std::string& input, int& context_len) -> replxx::Replxx::completions_t
  {
    context_len = static_cast<int>(input.size());
    replxx::Replxx::completions_t completions;
    for (const auto& name : registry_.completions(input))
    {
      completions.emplace_back(name);
    }
    return completions;
  });
}

void csm_terminal::setupHighlighter()
{
  repl_.set_highlighter_callback([this](const std::string& input, replxx::Replxx::colors_t& colors)
  {
    if (input.empty())
    {
      return;
    }

    std::size_t first_space = input.find(' ');
    if (first_space == std::string::npos)
    {
      first_space = input.size();
    }

    const std::string command = input.substr(0, first_space);
    const replxx::Replxx::Color color = registry_.hasCommand(command)
      ? replxx::Replxx::Color::GREEN
      : replxx::Replxx::Color::RED;

    for (std::size_t i = 0; i < first_space && i < colors.size(); ++i)
    {
      colors[i] = color;
    }
  });
}

void csm_terminal::setupBuiltins()
{
  registerCommand("help", [this](const std::vector<std::string>&)
  {
    for (const auto& name : registry_.commandNames())
    {
      std::cout << name << " - " << registry_.description(name) << "\n";
    }
    return 0;
  }, "help - list all available commands");

  registerCommand("history", [this](const std::vector<std::string>&) -> int
  {
     auto scan = repl_.history_scan();
     int index = 1;
     while (scan.next())
     {
       std::string line;
       std::cout << index << "  " << escapeOutput(line) << "\n";
       ++index;
     }

     if (index == 1) { std::cout << "History is empty\n"; }

     return 0;
 }, "history - show command history");

  registerCommand("clear", [](const std::vector<std::string>&)
  {
    std::cout << "\x1b[2J\x1b[H";
    return 0;
  }, "clear - clear the terminal screen");

  registerCommand("quit", [this](const std::vector<std::string>&)
  {
    stop();
    return 0;
  }, "quit - exit the terminal");

  registerCommand("echo", [](const std::vector<std::string>& args)
  {
    for (std::size_t i = 0; i < args.size(); ++i)
    {
      std::cout << escapeOutput(args[i]) << (i + 1 < args.size() ? " " : "");
    }
    std::cout << "\n";
    return 0;
  }, "echo [text...] - print arguments back to the terminal");

  registerCommand("version", [](const std::vector<std::string>&)
  {
    std::cout << "csm_cmd terminal 1.0.0\n";
    return 0;
  }, "version - print terminal version");

  registerCommand("time", [](const std::vector<std::string>&)
  {
    const std::time_t now = std::time(nullptr);
    std::tm tm_buf{};
#if defined(_WIN32)
    localtime_s(&tm_buf, &now);
#else
    localtime_r(&now, &tm_buf);
#endif
    std::cout << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S") << "\n";
    return 0;
  }, "time - print the current date and time");
}

void csm_terminal::loadHistory()
{
  repl_.history_load(history_path_);
}

void csm_terminal::saveHistory()
{
  repl_.history_save(history_path_);
}

int csm_terminal::executeWithTimeout(const std::string& name, const std::vector<std::string>& args)
{
  auto future = std::async(std::launch::async, [this, name, args]()
  {
    return registry_.execute(name, args);
  });

  const auto status = future.wait_for(timeout_);
  if (status != std::future_status::ready)
  {
    Logger::instance().warn("command timed out after " + std::to_string(timeout_.count()) + "ms: " + name);
    std::cerr << "error: command '" << escapeOutput(name) << "' timed out\n";
    return -2;
  }

  return future.get();
}

int csm_terminal::dispatch(const std::vector<std::string>& tokens)
{
  if (tokens.empty())
  {
    return 0;
  }

  const std::string& name = tokens.front();
  const std::vector<std::string> args(tokens.begin() + 1, tokens.end());

  if (!registry_.hasCommand(name))
  {
    Logger::instance().warn("rejected unknown command: " + name);
    std::cerr << "error: unknown command '" << escapeOutput(name) << "' (not in whitelist)\n";
    return -1;
  }

  Logger::instance().info("executing command: " + name);

  try
  {
    return executeWithTimeout(name, args);
  }
  catch (const std::exception& ex)
  {
    Logger::instance().error("command '" + name + "' threw: " + ex.what());
    std::cerr << "error: command '" << escapeOutput(name) << "' failed: " << escapeOutput(ex.what()) << "\n";
    return -3;
  }
}

void csm_terminal::run()
{
  running_ = true;

  while (running_)
  {
    const char* raw_line = repl_.input("csm> ");
    if (raw_line == nullptr)
    {
      break;
    }

    const std::string line(raw_line);
    if (line.empty())
    {
      continue;
    }

    std::vector<std::string> tokens;
    try
    {
      tokens = parser_.parse(line);
    }
    catch (const ParseError& ex)
    {
      Logger::instance().warn(std::string("parse error: ") + ex.what());
      std::cerr << "error: " << escapeOutput(ex.what()) << "\n";
      continue;
    }

    if (tokens.empty())
    {
      continue;
    }

    repl_.history_add(line);
    dispatch(tokens);
  }

  saveHistory();
}

void csm_terminal::stop()
{
  running_ = false;
}

}  // namespace csm_cmd
