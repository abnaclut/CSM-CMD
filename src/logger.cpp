#include <logger.hpp>

#include <cstdio>
#include <ctime>
#include <iomanip>
#include <iostream>  // std::clog
#include <sstream>

namespace csm_cmd
{

  Logger& Logger::instance()
  {
    static Logger logger;
    return logger;
  }

  Logger::~Logger()
  {
    if (stream_.is_open())
    {
      stream_.flush();
      stream_.close();
    }
  }

  void Logger::configure(const std::string& file_path,
                         const std::size_t max_bytes,
                         const int max_backups)
  {
    std::lock_guard<std::mutex> lock(mutex_);

    file_path_ = file_path;
    max_bytes_ = (max_bytes > 0) ? max_bytes : 1024 * 1024;
    max_backups_ = (max_backups > 0) ? max_backups : 3;

    stream_.open(file_path_, std::ios::app);
    configured_ = stream_.is_open();

    if (!configured_) { std::clog << "[ERROR] Failed to open log file: " << file_path_ << "\n"; }
  }

  std::string Logger::levelToString(const LogLevel level)
  {
    switch (level)
    {
      case LogLevel::kDebug:
        return "DEBUG";
      case LogLevel::kInfo:
        return "INFO";
      case LogLevel::kWarn:
        return "WARN";
      case LogLevel::kError:
        return "ERROR";
      default:
        return "UNKNOWN";
    }
  }

  std::string Logger::timestamp()
  {
    const auto now = std::chrono::system_clock::now();
    const std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf{};
  #if defined(_WIN32)
    localtime_s(&tm_buf, &now_time);
  #else
    localtime_r(&now_time, &tm_buf);
  #endif
    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
    return oss.str();
  }

  void Logger::rotateIfNeeded()
  {
    if (!configured_ || !stream_.is_open()) { return; }

    const auto pos = stream_.tellp();
    if (pos == std::streampos(-1)) { return; }

    if (static_cast<std::size_t>(pos) < max_bytes_) { return; }

    stream_.close();

    // delete oldest backup if it exists
    if (max_backups_ > 0)
    {
      const std::string oldest = file_path_ + "." + std::to_string(max_backups_);
      std::remove(oldest.c_str());

      // shift backups
      for (int i = max_backups_ - 1; i >= 1; --i)
      {
        std::string src = file_path_ + "." + std::to_string(i);
        std::string dst = file_path_ + "." + std::to_string(i + 1);
        if (std::rename(src.c_str(), dst.c_str()) != 0)
        {
          // Non-critical error, continue
          std::clog << "[WARN] Failed to rename backup: " << src << " -> " << dst << "\n";
        }
      }

      // rename current log to .1
      if (std::rename(file_path_.c_str(), (file_path_ + ".1").c_str()) != 0)
      {
        std::clog << "[WARN] Failed to rename current log to backup" << "\n";
        // Try to recover: re-open original file
        stream_.open(file_path_, std::ios::app);
        configured_ = stream_.is_open();
        return;
      }
    }

    // open new log
    stream_.open(file_path_, std::ios::app);
    configured_ = stream_.is_open();

    if (!configured_) { std::clog << "[ERROR] Failed to re-open log file after rotation: " << file_path_ << "\n"; }
  }

  void Logger::log(const LogLevel level, const std::string& message)
  {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!configured_)
    {
      // Log to stderr as fallback when logger isn't configured
      std::clog << "[LOG] " << message << "\n";
      return;
    }

    rotateIfNeeded();
    stream_ << "[" << timestamp() << "] [" << levelToString(level) << "] " << message << "\n";
    stream_.flush();
  }

  void Logger::debug(const std::string& message) { log(LogLevel::kDebug, message); }
  void Logger::info(const std::string& message) { log(LogLevel::kInfo, message); }
  void Logger::warn(const std::string& message) { log(LogLevel::kWarn, message); }
  void Logger::error(const std::string& message) { log(LogLevel::kError, message); }

}  // namespace csm_cmd