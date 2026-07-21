#pragma once
#include "log_error.h"
#include <chrono>
#include <expected>
#include <memory>
#include <string>
#include <vector>

class Sink;

class Logger {
public:
  Logger(size_t batch_size, std::chrono::milliseconds interval,
         std::vector<std::unique_ptr<Sink>> &&sinks,
         LogLevel log_level = LogLevel::Info);
  ~Logger();
  Logger(Logger &&) = delete;
  Logger &operator=(Logger &&) = delete;
  Logger(const Logger &) = delete;
  Logger &operator=(const Logger &) = delete;
  void log(const std::string &message, LogLevel log_level = LogLevel::Info);
  std::expected<void, std::vector<LogError>> flush();
  void set_log_level(LogLevel);

private:
  struct Impl;
  std::unique_ptr<Impl> pimpl_;
  void worker_loop();
};
