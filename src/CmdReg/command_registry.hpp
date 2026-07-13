#ifndef CSM_CMD_COMMAND_REGISTRY_HPP
#define CSM_CMD_COMMAND_REGISTRY_HPP

#include <functional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace csm_cmd
{

/**
 * @brief Thrown when executing an unregistered or malformed command.
 */
class CommandError : public std::runtime_error
{
public:
  explicit CommandError(const std::string& message)
    : std::runtime_error(message)
  {
  }
};

/**
 * @brief Holds the whitelist of registered commands and their aliases.
 *
 * A command may only be executed if it was explicitly registered through
 * registerCommand(). No arbitrary or system command execution is possible.
 */
class CommandRegistry
{
public:
  using CommandHandler = std::function<int(const std::vector<std::string>&)>;

  struct CommandInfo
  {
    std::string name;
    std::string description;
    CommandHandler handler;
  };

  /**
   * @brief Register a new command handler under the whitelist.
   * @throws CommandError if the name is empty or already registered.
   */
  void registerCommand(const std::string& name, CommandHandler handler, const std::string& description);

  /**
   * @brief Register an alias that resolves to an already registered command.
   * @throws CommandError if the target command does not exist.
   */
  void registerAlias(const std::string& alias, const std::string& target);

  /**
   * @brief Check whether a name (command or alias) is whitelisted.
   */
  bool hasCommand(const std::string& name) const;

  /**
   * @brief Execute a whitelisted command by name.
   * @throws CommandError if the command is not registered.
   */
  int execute(const std::string& name, const std::vector<std::string>& args) const;

  /**
   * @brief Return the human readable description for a command.
   */
  std::string description(const std::string& name) const;

  /**
   * @brief List all registered command names, sorted alphabetically.
   */
  std::vector<std::string> commandNames() const;

  /**
   * @brief Return command names starting with the given case-sensitive prefix.
   */
  std::vector<std::string> completions(const std::string& prefix) const;

private:
  std::string resolveAlias(const std::string& name) const;

  std::unordered_map<std::string, CommandInfo> commands_;
  std::unordered_map<std::string, std::string> aliases_;
};

}  // namespace csm_cmd

#endif  // CSM_CMD_COMMAND_REGISTRY_HPP
