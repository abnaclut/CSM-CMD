#include <gtest/gtest.h>

#include <cmd_registry.hpp>

using csm_cmd::CommandError;
using csm_cmd::CommandRegistry;

TEST(CommandRegistryTest, RegistersAndExecutesCommand)
{
  CommandRegistry registry;
  registry.registerCommand("greet", [](const std::vector<std::string>& args)
  {
    return args.empty() ? 1 : 0;
  }, "greet [name]");

  EXPECT_TRUE(registry.hasCommand("greet"));
  EXPECT_EQ(registry.execute("greet", {"World"}), 0);
  EXPECT_EQ(registry.execute("greet", {}), 1);
}

TEST(CommandRegistryTest, RejectsDuplicateRegistration)
{
  CommandRegistry registry;
  registry.registerCommand("greet", [](const std::vector<std::string>&) { return 0; }, "");
  EXPECT_THROW(registry.registerCommand("greet", [](const std::vector<std::string>&) { return 0; }, ""), CommandError);
}

TEST(CommandRegistryTest, RejectsEmptyName)
{
  CommandRegistry registry;
  EXPECT_THROW(registry.registerCommand("", [](const std::vector<std::string>&) { return 0; }, ""), CommandError);
}

TEST(CommandRegistryTest, RejectsNullHandler)
{
  CommandRegistry registry;
  EXPECT_THROW(registry.registerCommand("test", nullptr, ""), CommandError);
}

TEST(CommandRegistryTest, ThrowsOnUnknownCommand)
{
  CommandRegistry registry;
  EXPECT_THROW(registry.execute("missing", {}), CommandError);
  EXPECT_FALSE(registry.hasCommand("missing"));
}

TEST(CommandRegistryTest, AliasResolvesToTarget)
{
  CommandRegistry registry;
  registry.registerCommand("list", [](const std::vector<std::string>&) { return 42; }, "list items");
  registry.registerAlias("ls", "list");

  EXPECT_TRUE(registry.hasCommand("ls"));
  EXPECT_EQ(registry.execute("ls", {}), 42);
}

TEST(CommandRegistryTest, AliasToUnknownTargetThrows)
{
  CommandRegistry registry;
  EXPECT_THROW(registry.registerAlias("ls", "list"), CommandError);
}

TEST(CommandRegistryTest, AliasCannotBeSameAsTarget)
{
  CommandRegistry registry;
  registry.registerCommand("test", [](const std::vector<std::string>&) { return 0; }, "");
  EXPECT_THROW(registry.registerAlias("test", "test"), CommandError);
}

TEST(CommandRegistryTest, AliasDuplicateThrows)
{
  CommandRegistry registry;
  registry.registerCommand("list", [](const std::vector<std::string>&) { return 0; }, "");
  registry.registerAlias("ls", "list");
  EXPECT_THROW(registry.registerAlias("ls", "list"), CommandError);
}

TEST(CommandRegistryTest, GetDescriptionReturnsCorrectDescription)
{
  CommandRegistry registry;
  registry.registerCommand("test", [](const std::vector<std::string>&) { return 0; }, "Test description");

  EXPECT_EQ(registry.getDescription("test"), "Test description");
  EXPECT_EQ(registry.getDescription("missing"), "");
}

TEST(CommandRegistryTest, GetDescriptionResolvesAlias)
{
  CommandRegistry registry;
  registry.registerCommand("list", [](const std::vector<std::string>&) { return 0; }, "List items");
  registry.registerAlias("ls", "list");

  EXPECT_EQ(registry.getDescription("ls"), "List items");
}

TEST(CommandRegistryTest, GetCommandNamesReturnsSortedNames)
{
  CommandRegistry registry;
  registry.registerCommand("zebra", [](const std::vector<std::string>&) { return 0; }, "");
  registry.registerCommand("apple", [](const std::vector<std::string>&) { return 0; }, "");
  registry.registerCommand("banana", [](const std::vector<std::string>&) { return 0; }, "");

  const auto names = registry.getCommandNames();
  ASSERT_EQ(names.size(), 3u);
  EXPECT_EQ(names[0], "apple");
  EXPECT_EQ(names[1], "banana");
  EXPECT_EQ(names[2], "zebra");
}

TEST(CommandRegistryTest, GetCommandsReturnsConstReference)
{
  CommandRegistry registry;
  registry.registerCommand("test", [](const std::vector<std::string>&) { return 0; }, "Test");

  const auto& commands = registry.getCommands();
  EXPECT_EQ(commands.size(), 1u);
  EXPECT_TRUE(commands.contains("test"));
}

TEST(CommandRegistryTest, SizeReturnsCorrectCount)
{
  CommandRegistry registry;
  EXPECT_EQ(registry.size(), 0u);

  registry.registerCommand("test1", [](const std::vector<std::string>&) { return 0; }, "");
  EXPECT_EQ(registry.size(), 1u);

  registry.registerCommand("test2", [](const std::vector<std::string>&) { return 0; }, "");
  EXPECT_EQ(registry.size(), 2u);

  registry.registerAlias("t1", "test1");
  EXPECT_EQ(registry.size(), 2u);  // Aliases don't increase count
}

TEST(CommandRegistryTest, ClearRemovesAllCommandsAndAliases)
{
  CommandRegistry registry;
  registry.registerCommand("test1", [](const std::vector<std::string>&) { return 0; }, "");
  registry.registerCommand("test2", [](const std::vector<std::string>&) { return 0; }, "");
  registry.registerAlias("t1", "test1");

  registry.clear();

  EXPECT_EQ(registry.size(), 0u);
  EXPECT_FALSE(registry.hasCommand("test1"));
  EXPECT_FALSE(registry.hasCommand("test2"));
  EXPECT_FALSE(registry.hasCommand("t1"));
}

TEST(CommandRegistryTest, CompletionsAreCaseSensitivePrefixMatch)
{
  CommandRegistry registry;
  registry.registerCommand("Help", [](const std::vector<std::string>&) { return 0; }, "");
  registry.registerCommand("help", [](const std::vector<std::string>&) { return 0; }, "");
  registry.registerCommand("history", [](const std::vector<std::string>&) { return 0; }, "");

  const auto lower = registry.getCompletions("he");
  ASSERT_EQ(lower.size(), 1u);
  EXPECT_EQ(lower[0], "help");

  const auto upper = registry.getCompletions("He");
  ASSERT_EQ(upper.size(), 1u);
  EXPECT_EQ(upper[0], "Help");
}

TEST(CommandRegistryTest, CompletionsIncludeAliases)
{
  CommandRegistry registry;
  registry.registerCommand("list", [](const std::vector<std::string>&) { return 0; }, "");
  registry.registerAlias("ls", "list");
  registry.registerAlias("l", "list");

  const auto completions = registry.getCompletions("l");
  // Should include "list", "ls", "l"
  EXPECT_GE(completions.size(), 3u);
}

TEST(CommandRegistryTest, CompletionsEmptyForUnknownPrefix)
{
  CommandRegistry registry;
  registry.registerCommand("help", [](const std::vector<std::string>&) { return 0; }, "");
  registry.registerCommand("history", [](const std::vector<std::string>&) { return 0; }, "");

  EXPECT_TRUE(registry.getCompletions("zzz").empty());
}

TEST(CommandRegistryTest, CompletionsSorted)
{
  CommandRegistry registry;
  registry.registerCommand("alpha", [](const std::vector<std::string>&) { return 0; }, "");
  registry.registerCommand("beta", [](const std::vector<std::string>&) { return 0; }, "");
  registry.registerCommand("gamma", [](const std::vector<std::string>&) { return 0; }, "");

  const auto completions = registry.getCompletions("");
  ASSERT_EQ(completions.size(), 3u);
  EXPECT_EQ(completions[0], "alpha");
  EXPECT_EQ(completions[1], "beta");
  EXPECT_EQ(completions[2], "gamma");
}