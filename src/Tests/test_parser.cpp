#include <gtest/gtest.h>

#include <cmd_parser.hpp>

using csm_cmd::CommandParser;
using csm_cmd::ParseError;

TEST(CommandParserTest, SplitsSimpleWhitespace)
{
  const auto tokens = CommandParser::parse("echo hello world");
  ASSERT_EQ(tokens.size(), 3u);
  EXPECT_EQ(tokens[0], "echo");
  EXPECT_EQ(tokens[1], "hello");
  EXPECT_EQ(tokens[2], "world");
}

TEST(CommandParserTest, HandlesMultipleSpaces)
{
  const auto tokens = CommandParser::parse("echo   hello    world");
  ASSERT_EQ(tokens.size(), 3u);
  EXPECT_EQ(tokens[0], "echo");
  EXPECT_EQ(tokens[1], "hello");
  EXPECT_EQ(tokens[2], "world");
}

TEST(CommandParserTest, HandlesDoubleQuotedSpaces)
{
  const auto tokens = CommandParser::parse("echo \"hello world\"");
  ASSERT_EQ(tokens.size(), 2u);
  EXPECT_EQ(tokens[1], "hello world");
}

TEST(CommandParserTest, HandlesSingleQuotedSpaces)
{
  const auto tokens = CommandParser::parse("echo 'hello world'");
  ASSERT_EQ(tokens.size(), 2u);
  EXPECT_EQ(tokens[1], "hello world");
}

TEST(CommandParserTest, HandlesMixedQuotes)
{
  const auto tokens = CommandParser::parse("echo \"hello 'world'\"");
  ASSERT_EQ(tokens.size(), 2u);
  EXPECT_EQ(tokens[1], "hello 'world'");
}

TEST(CommandParserTest, HandlesEscapedSpace)
{
  const auto tokens = CommandParser::parse("echo hello\\ world");
  ASSERT_EQ(tokens.size(), 2u);
  EXPECT_EQ(tokens[1], "hello world");
}

TEST(CommandParserTest, HandlesEscapedBackslash)
{
  const auto tokens = CommandParser::parse("echo hello\\\\world");
  ASSERT_EQ(tokens.size(), 2u);
  EXPECT_EQ(tokens[1], "hello\\world");
}

TEST(CommandParserTest, HandlesEscapedQuoteInsideDoubleQuotes)
{
  const auto tokens = CommandParser::parse(R"(echo "say \"hi\"")");
  ASSERT_EQ(tokens.size(), 2u);
  EXPECT_EQ(tokens[1], "say \"hi\"");
}

TEST(CommandParserTest, HandlesEscapedBackslashInsideDoubleQuotes)
{
  const auto tokens = CommandParser::parse(R"(echo "hello\\world")");
  ASSERT_EQ(tokens.size(), 2u);
  EXPECT_EQ(tokens[1], "hello\\world");
}

TEST(CommandParserTest, HandlesEmptyDoubleQuotes)
{
  const auto tokens = CommandParser::parse("echo \"\"");
  ASSERT_EQ(tokens.size(), 2u);
  EXPECT_EQ(tokens[0], "echo");
  EXPECT_EQ(tokens[1], "");
}

TEST(CommandParserTest, HandlesEmptySingleQuotes)
{
  const auto tokens = CommandParser::parse("echo ''");
  ASSERT_EQ(tokens.size(), 2u);
  EXPECT_EQ(tokens[0], "echo");
  EXPECT_EQ(tokens[1], "");
}

TEST(CommandParserTest, HandlesBackslashAtEnd)
{
  const auto tokens = CommandParser::parse("echo hello\\");
  ASSERT_EQ(tokens.size(), 2u);
  EXPECT_EQ(tokens[1], "hello\\");
}

TEST(CommandParserTest, ThrowsOnUnterminatedDoubleQuote)
{
  EXPECT_THROW(CommandParser::parse("echo \"unterminated"), ParseError);
}

TEST(CommandParserTest, ThrowsOnUnterminatedSingleQuote)
{
  EXPECT_THROW(CommandParser::parse("echo 'unterminated"), ParseError);
}

TEST(CommandParserTest, ThrowsOnOversizedInput)
{
  const std::string huge(CommandParser::kMaxInputLength + 1, 'a');
  EXPECT_THROW(CommandParser::parse(huge), ParseError);
}

TEST(CommandParserTest, EmptyLineReturnsNoTokens)
{
  EXPECT_TRUE(CommandParser::parse("").empty());
  EXPECT_TRUE(CommandParser::parse("   ").empty());
}

TEST(CommandParserTest, OnlyQuotesReturnsEmptyToken)
{
  const auto tokens = CommandParser::parse("\"\"");
  ASSERT_EQ(tokens.size(), 1u);
  EXPECT_EQ(tokens[0], "");
}