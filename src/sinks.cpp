#include "sinks.h"
#include "log_error.h"
#include <fstream>
#include <ios>
#include <iostream>
#include <span>
#include <stdexcept>

Sink::~Sink() = default;

std::string Sink::log_lvl_to_string(LogLevel log_level) {
  switch (log_level) {
  case LogLevel::Debug:
    return "[DEBUG]";

  case LogLevel::Info:
    return "[INFO]";

  case LogLevel::Warning:
    return "[WARNING]";

  case LogLevel::Error:
    return "[ERROR]";

  default:
    throw std::invalid_argument("Invalid argument");
  }
}

void Sink::filter(std::span<const LogEntry> lines) {
  for (const LogEntry &log : lines) {
    if (log.log_level >= this->min_level_) {
      do_write(log_lvl_to_string(log.log_level) + " " + log.msg + "\n");
    }
  }
}

struct FileSink::Impl {
  std::ofstream file;
  std::string filename;
};

FileSink::~FileSink() = default;

FileSink::FileSink(const std::string &filename, const LogLevel min_level)
    : Sink(min_level), pimpl_(std::make_unique<Impl>()) {
  pimpl_->filename = filename;
  pimpl_->file.open(filename, std::ios::app);
}

std::expected<void, LogError> FileSink::write(std::span<const LogEntry> lines) {
  if (!pimpl_->file) {
    return std::unexpected(LogError::FileNotOpen);
  }
  filter(lines);
  pimpl_->file.flush();
  if (!pimpl_->file) {
    return std::unexpected(LogError::WriteFailed);
  }
  return {};
}

void FileSink::do_write(const std::string &line) { pimpl_->file << line; }

ConsoleSink::ConsoleSink(const LogLevel min_level) : Sink(min_level) {}

ConsoleSink::~ConsoleSink() = default;

std::expected<void, LogError>
ConsoleSink::write(std::span<const LogEntry> lines) {
  filter(lines);
  return {};
}

void ConsoleSink::do_write(const std::string &line) { std::cout << line; }

NotifierSink::NotifierSink(const LogLevel min_level) : Sink(min_level) {}

NotifierSink::~NotifierSink() = default;

std::expected<void, LogError>
NotifierSink::write(std::span<const LogEntry> lines) {
  filter(lines);
  return {};
}

void NotifierSink::do_write(const std::string &line) { std::cerr << line; }
