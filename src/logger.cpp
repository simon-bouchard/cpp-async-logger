#include "logger.h"
#include "log_error.h"
#include "sinks.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

struct Logger::Impl {
  std::vector<std::unique_ptr<Sink>> sinks;
  size_t batch_size;
  std::vector<LogEntry> buffer;
  std::chrono::milliseconds interval;
  std::mutex m;
  std::condition_variable cv;
  std::thread worker;
  bool stop = false;
  std::atomic<LogLevel> log_level;
};

Logger::Logger(size_t batch_size, std::chrono::milliseconds interval,
               std::vector<std::unique_ptr<Sink>> &&sinks, LogLevel log_level)
    : pimpl_(std::make_unique<Impl>()) {
  pimpl_->sinks = std::move(sinks);
  pimpl_->batch_size = batch_size;
  pimpl_->buffer.reserve(batch_size);
  pimpl_->interval = interval;
  pimpl_->log_level = log_level;
  pimpl_->worker = std::thread(&Logger::worker_loop, this);
}

void Logger::set_log_level(LogLevel log_level) {
  pimpl_->log_level = log_level;
}

Logger::~Logger() {
  {
    std::lock_guard<std::mutex> lock(pimpl_->m);
    pimpl_->stop = true;
  }
  pimpl_->cv.notify_one();
  pimpl_->worker.join();
}

void Logger::log(const std::string &message, LogLevel log_level) {
  if (log_level < pimpl_->log_level) {
    return;
  }

  std::unique_lock<std::mutex> lock(pimpl_->m);
  pimpl_->buffer.push_back(LogEntry{message, log_level});
  bool should_notify = (pimpl_->buffer.size() >= pimpl_->batch_size);
  lock.unlock();
  if (should_notify) {
    pimpl_->cv.notify_one();
  }
}

std::expected<void, std::vector<LogError>> Logger::flush() {
  std::vector<LogEntry> batch;
  {
    std::lock_guard<std::mutex> lock(pimpl_->m);
    batch = std::move(pimpl_->buffer);
    pimpl_->buffer.clear();
  }
  std::vector<LogError> errors;
  for (std::unique_ptr<Sink> &sink : pimpl_->sinks) {
    auto result = sink->write(batch);
    if (!result) {
      errors.push_back(result.error());
    }
  }
  if (!errors.empty()) {
    return std::unexpected(errors);
  }
  return {};
}

void Logger::worker_loop() {
  while (true) {
    std::unique_lock<std::mutex> lock(pimpl_->m);
    pimpl_->cv.wait_for(lock, pimpl_->interval, [&]() {
      return pimpl_->buffer.size() >= pimpl_->batch_size || pimpl_->stop;
    });

    if (pimpl_->buffer.empty() && pimpl_->stop) {
      break;
    }
    lock.unlock();
    auto result = flush();
    if (!result) {
      for (LogError &error : result.error()) {
        switch (error) {
        case LogError::FileNotOpen:
          std::cerr << "File not open\n";
          break;
        case LogError::WriteFailed:
          std::cerr << "Write failed\n";
          break;
        }
      }
    }
  }
}
