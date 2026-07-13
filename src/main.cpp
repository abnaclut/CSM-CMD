#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include "CsmTerminal/csm_terminal.hpp"

namespace
{
  void printUsage(const char* prog_name)
  {
    std::cout << "usage: " << prog_name << " -nogui [--timeout-ms N]\n";
  }
}  // namespace

int main(const int argc, char** argv)
{
  bool nogui = true; //default for terminal, change for gui derivatives
  std::chrono::milliseconds timeout = csm_cmd::csm_terminal::kDefaultTimeout;

  for (int i = 1; i < argc; ++i)
  {
    const std::string arg = argv[i];
    if (arg == "-nogui")
    {
      nogui = true;
    }
    else if (arg == "--timeout-ms" && i + 1 < argc)
    {
      timeout = std::chrono::milliseconds(std::stoi(argv[++i]));
    }
    else if (arg == "--help" || arg == "-h")
    {
      printUsage(argv[0]);
      return 0;
    }
  }

  if (!nogui)
  {
    printUsage(argv[0]);
    return 1;
  }

  csm_cmd::csm_terminal terminal(timeout);

  terminal.registerCommand("greet", [](const std::vector<std::string>& args)
  {
    std::cout << "Hello, " << (args.empty() ? "World" : args[0]) << "!\n";
    return 0;
  }, "greet [name] - Say hello");

  terminal.registerCommand("list", [](const std::vector<std::string>&)
  {
    std::cout << "item1\nitem2\nitem3\n";
    return 0;
  }, "list - list demo items");

  terminal.registerAlias("ls", "list");

  terminal.registerCommand("slow", [](const std::vector<std::string>&)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    std::cout << "finally done\n";
    return 0;
  }, "slow - demonstrates timeout handling (sleeps 500ms)");

  terminal.run();

  return 0;
}
