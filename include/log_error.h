#pragma once
#include <string>

enum class LogError {
  FileNotOpen,
  WriteFailed,
};

enum class LogLevel { Debug, Info, Warning, Error };

struct LogEntry {
  std::string msg;
  LogLevel log_level;
};
