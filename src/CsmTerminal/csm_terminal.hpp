#ifndef CSM_CMD_CSM_TERMINAL_HPP
#define CSM_CMD_CSM_TERMINAL_HPP

#include <chrono>
#include <memory>
#include <string>

#include <replxx.hxx>

#include "../CmdParser/command_parser.hpp"
#include "../CmdReg/command_registry.hpp"

namespace csm_cmd
{

/**
 * @brief Interactive replxx-backed terminal for the -nogui command mode.
 *
 * Owns the readline loop, history persistence, case-sensitive tab
 * completion and per-command timeout enforcement. Only commands explicitly
 * registered through registerCommand()/registerAlias() can be executed;
 * there is no path to system()/exec*() or arbitrary shell execution.
 */
class csm_terminal
{
public:
  static constexpr std::chrono::milliseconds kDefaultTimeout{100};
  static constexpr std::size_t kMaxHistoryLines = 1000;

  explicit csm_terminal(std::chrono::milliseconds command_timeout = kDefaultTimeout);
  ~csm_terminal();

  csm_terminal(const csm_terminal&) = delete;
  csm_terminal& operator=(const csm_terminal&) = delete;

  /**
   * @brief Register a whitelisted command handler.
   */
  void registerCommand(const std::string& name, CommandRegistry::CommandHandler handler, const std::string& description);

  /**
   * @brief Register an alias for an already registered command.
   */
  void registerAlias(const std::string& alias, const std::string& target);

  /**
   * @brief Run the blocking read-eval-print loop until quit or SIGINT.
   */
  void run();

  /**
   * @brief Request the run loop to stop after the current command.
   */
  void stop();

private:
  void setupBuiltins();
  void setupCompletion();
  void setupHighlighter();
  void loadHistory();
  void saveHistory();
  int dispatch(const std::vector<std::string>& tokens);
  int executeWithTimeout(const std::string& name, const std::vector<std::string>& args);

  static std::string escapeOutput(const std::string& text);
  static void installSignalHandler(csm_terminal* self);
  static void signalHandler(int signum);

  replxx::Replxx repl_;
  CommandRegistry registry_;
  CommandParser parser_;
  std::chrono::milliseconds timeout_;
  std::string history_path_;
  bool running_ = false;

  static csm_terminal* active_instance_;
};

}  // namespace csm_cmd

#endif  // CSM_CMD_CSM_TERMINAL_HPP
