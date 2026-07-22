#include <gtest/gtest.h>

#include <cmd_registration.hpp>
#include <csm_terminal.hpp>

using csm_cmd::CommandRegistrar;
using csm_cmd::Terminal;
using csm_cmd::initTerminal;
using csm_cmd::isTerminalInitialized;

class RegistrationTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    CommandRegistrar::clearCommands();
  }

  void TearDown() override
  {
    CommandRegistrar::clearCommands();
  }
};

TEST_F(RegistrationTest, RegistersCommandBeforeInit)
{
  bool executed = false;

  CommandRegistrar::regCmd("test", [&executed](const std::vector<std::string>&) -> int
  {
    executed = true;
    return 0;
  }, "Test command");

  EXPECT_FALSE(executed);
  EXPECT_FALSE(CommandRegistrar::isInitialized());

  initTerminal();

  EXPECT_TRUE(CommandRegistrar::isInitialized());
  EXPECT_FALSE(executed); // Command not executed, just registered

  // Verify command is registered by checking via Terminal
  auto& terminal = Terminal::instance();
  EXPECT_TRUE(terminal.getRegistry().hasCommand("test"));
}

TEST_F(RegistrationTest, RegistersCommandAfterInit)
{
  initTerminal();

  bool executed = false;
  CommandRegistrar::regCmd("test", [&executed](const std::vector<std::string>&) -> int
  {
    executed = true;
    return 0;
  }, "Test command");

  EXPECT_TRUE(CommandRegistrar::isInitialized());
  EXPECT_FALSE(executed);

  auto& terminal = Terminal::instance();
  EXPECT_TRUE(terminal.getRegistry().hasCommand("test"));
}

TEST_F(RegistrationTest, RegistersAliasBeforeInit)
{
  initTerminal();

  CommandRegistrar::regCmd("list", [](const std::vector<std::string>&) -> int
  {
    return 0;
  }, "List items");

  CommandRegistrar::regAlias("ls", "list");

  auto& terminal = Terminal::instance();
  EXPECT_TRUE(terminal.getRegistry().hasCommand("ls"));
}

TEST_F(RegistrationTest, RegisterDuplicateCommandThrows)
{
  initTerminal();

  CommandRegistrar::regCmd("test", [](const std::vector<std::string>&) -> int
  {
    return 0;
  }, "Test");

  EXPECT_THROW(
    CommandRegistrar::regCmd("test", [](const std::vector<std::string>&) -> int
    {
      return 0;
    }, "Test duplicate"),
    std::runtime_error
  );
}

TEST_F(RegistrationTest, RegisterAliasToUnknownTargetThrows)
{
  initTerminal();

  EXPECT_THROW(
    CommandRegistrar::regAlias("ls", "unknown"),
    std::runtime_error
  );
}

TEST_F(RegistrationTest, RegisterDuplicateAliasThrows)
{
  initTerminal();

  CommandRegistrar::regCmd("list", [](const std::vector<std::string>&) -> int
  {
    return 0;
  }, "List");

  CommandRegistrar::regAlias("ls", "list");

  EXPECT_THROW(
    CommandRegistrar::regAlias("ls", "list"),
    std::runtime_error
  );
}

TEST_F(RegistrationTest, RegisterEmptyNameThrows)
{
  EXPECT_THROW(
    CommandRegistrar::regCmd("", [](const std::vector<std::string>&) -> int
    {
      return 0;
    }, ""),
    std::invalid_argument
  );
}

TEST_F(RegistrationTest, RegisterNullHandlerThrows)
{
  EXPECT_THROW(
    CommandRegistrar::regCmd("test", nullptr, ""),
    std::invalid_argument
  );
}

TEST_F(RegistrationTest, RegisterEmptyAliasThrows)
{
  EXPECT_THROW(
    CommandRegistrar::regAlias("", "test"),
    std::invalid_argument
  );
}

TEST_F(RegistrationTest, RegisterAliasEmptyTargetThrows)
{
  EXPECT_THROW(
    CommandRegistrar::regAlias("ls", ""),
    std::invalid_argument
  );
}

TEST_F(RegistrationTest, ClearCommandsClearsAll)
{
  initTerminal();

  CommandRegistrar::regCmd("test1", [](const std::vector<std::string>&) -> int
  {
    return 0;
  }, "Test1");

  CommandRegistrar::regCmd("test2", [](const std::vector<std::string>&) -> int
  {
    return 0;
  }, "Test2");

  CommandRegistrar::regAlias("t1", "test1");

  CommandRegistrar::clearCommands();

  auto& terminal = Terminal::instance();
  EXPECT_FALSE(terminal.getRegistry().hasCommand("test1"));
  EXPECT_FALSE(terminal.getRegistry().hasCommand("test2"));
  EXPECT_FALSE(terminal.getRegistry().hasCommand("t1"));
}

TEST_F(RegistrationTest, GetCommandsReturnsCopy)
{
  initTerminal();

  CommandRegistrar::regCmd("test", [](const std::vector<std::string>&) -> int
  {
    return 0;
  }, "Test command");

  auto commands = CommandRegistrar::getCommands();
  EXPECT_EQ(commands.size(), 1u);
  EXPECT_TRUE(commands.contains("test"));
  EXPECT_EQ(commands["test"].description, "Test command");
}

TEST_F(RegistrationTest, IsInitializedReturnsCorrectState)
{
  EXPECT_FALSE(CommandRegistrar::isInitialized());

  initTerminal();

  EXPECT_TRUE(CommandRegistrar::isInitialized());
}