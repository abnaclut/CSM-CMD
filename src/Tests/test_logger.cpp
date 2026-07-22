#include <gtest/gtest.h>

#include <logger.hpp>
#include <fstream>
#include <filesystem>
#include <thread>
#include <chrono>

using csm_cmd::Logger;
using csm_cmd::LogLevel;

namespace fs = std::filesystem;

class LoggerTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    // Use unique path for each test using test name
    const auto* test_info = ::testing::UnitTest::GetInstance()->current_test_info();
    std::string test_name = test_info ? test_info->name() : "unknown";
    log_path_ = "test_log_" + test_name + ".txt";

    if (fs::exists(log_path_))
    {
      fs::remove(log_path_);
    }
  }

  void TearDown() override
  {
    if (fs::exists(log_path_))
    {
      fs::remove(log_path_);
    }
  }

  std::string log_path_;
};

TEST_F(LoggerTest, ConfigureOpensFile)
{
  auto& logger = Logger::instance();
  logger.configure(log_path_, 1024, 3);

  std::ifstream file(log_path_);
  EXPECT_TRUE(file.is_open());
}

TEST_F(LoggerTest, LogWritesMessageToFile)
{
  auto& logger = Logger::instance();
  logger.configure(log_path_, 1024, 3);
  logger.info("Test message");

  std::ifstream file(log_path_);
  std::string line;
  std::getline(file, line);

  EXPECT_FALSE(line.empty());
  EXPECT_TRUE(line.find("Test message") != std::string::npos);
}

TEST_F(LoggerTest, LogLevelsAreCorrect)
{
  auto& logger = Logger::instance();
  logger.configure(log_path_, 1024, 3);

  logger.debug("Debug message");
  logger.info("Info message");
  logger.warn("Warn message");
  logger.error("Error message");

  std::ifstream file(log_path_);
  std::string line;
  std::vector<std::string> lines;

  while (std::getline(file, line))
  {
    lines.push_back(line);
  }

  ASSERT_EQ(lines.size(), 4u);
  EXPECT_TRUE(lines[0].find("DEBUG") != std::string::npos);
  EXPECT_TRUE(lines[1].find("INFO") != std::string::npos);
  EXPECT_TRUE(lines[2].find("WARN") != std::string::npos);
  EXPECT_TRUE(lines[3].find("ERROR") != std::string::npos);
}

TEST_F(LoggerTest, RotationOccursWhenSizeExceeded)
{
  auto& logger = Logger::instance();
  logger.configure(log_path_, 100, 3);

  for (int i = 0; i < 10; ++i)
  {
    logger.info("This is a test message that should be long enough to trigger rotation");
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  bool has_backup = fs::exists(log_path_ + ".1");
  EXPECT_TRUE(has_backup);
}

TEST_F(LoggerTest, LogBeforeConfigureDoesNothing)
{
  auto& logger = Logger::instance();
  // Don't configure - should not write
  logger.info("This should not be written");

  EXPECT_FALSE(fs::exists(log_path_));
}

TEST_F(LoggerTest, SingletonReturnsSameInstance)
{
  auto& logger1 = Logger::instance();
  auto& logger2 = Logger::instance();

  EXPECT_EQ(&logger1, &logger2);
}

TEST_F(LoggerTest, ConfigureWithZeroMaxBytes)
{
  auto& logger = Logger::instance();
  EXPECT_NO_THROW(logger.configure(log_path_, 0, 3));
  logger.info("Test with zero max bytes");
  EXPECT_TRUE(fs::exists(log_path_));
}