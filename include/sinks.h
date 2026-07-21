#pragma once
#include "log_error.h"
#include <expected>
#include <memory>
#include <span>
#include <string>

class Sink {
public:
  explicit Sink(const LogLevel min_level) : min_level_(min_level) {};
  virtual ~Sink();
  virtual std::expected<void, LogError>
  write(std::span<const LogEntry> lines) = 0;

protected:
  void filter(std::span<const LogEntry> lines);

private:
  LogLevel min_level_;
  virtual void do_write(const std::string &line) = 0;
  std::string log_lvl_to_string(LogLevel min_level);
};

class FileSink : public Sink {
public:
  ~FileSink() override;
  FileSink(const std::string &filename,
           const LogLevel min_level = LogLevel::Info);
  FileSink(const FileSink &) = delete;
  FileSink &operator=(const FileSink &) = delete;
  std::expected<void, LogError> write(std::span<const LogEntry> lines) override;

private:
  struct Impl;
  std::unique_ptr<Impl> pimpl_;
  void do_write(const std::string &line) override;
};

class ConsoleSink : public Sink {
public:
  ~ConsoleSink() override;
  ConsoleSink(const LogLevel min_level = LogLevel::Info);
  ConsoleSink(const ConsoleSink &) = delete;
  ConsoleSink &operator=(const ConsoleSink &) = delete;
  std::expected<void, LogError> write(std::span<const LogEntry> lines) override;

private:
  void do_write(const std::string &line) override;
};

class NotifierSink : public Sink {
public:
  ~NotifierSink() override;
  NotifierSink(const LogLevel min_level = LogLevel::Error);
  NotifierSink(const NotifierSink &) = delete;
  NotifierSink &operator=(const NotifierSink &) = delete;
  std::expected<void, LogError> write(std::span<const LogEntry> lines) override;

private:
  void do_write(const std::string &line) override;
};
