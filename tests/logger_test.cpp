#include "log_error.h"
#include "logger.h"
#include "sinks.h"
#include <chrono>
#include <filesystem>
#include <format>
#include <fstream>
#include <gtest/gtest.h>
#include <memory>
#include <sstream>
#include <thread>
#include <type_traits>
#include <unordered_set>

using namespace std::chrono_literals;

class LoggerFixture : public ::testing::Test {
protected:
  void SetUp() override {
    file = "test_logs";
    old_buf = std::cout.rdbuf(captured.rdbuf());

    std::vector<std::unique_ptr<Sink>> sinks;
    sinks.push_back(std::make_unique<FileSink>(file));
    sinks.push_back(std::make_unique<ConsoleSink>());

    std::ofstream clear(file, std::ios::trunc);
    logger = std::make_unique<Logger>(100, std::chrono::milliseconds(5000),
                                      std::move(sinks), LogLevel::Info);
  }

  void TearDown() override {
    std::filesystem::remove(file);
    std::cout.rdbuf(old_buf);
  }

  std::unique_ptr<Logger> logger;
  std::string file;
  std::ostringstream captured;
  std::streambuf *old_buf;
};

class IntervalLoggerFixture : public ::testing::Test {
protected:
  void SetUp() override {
    file = "test_logs";
    old_buf = std::cout.rdbuf(captured.rdbuf());

    std::vector<std::unique_ptr<Sink>> sinks;
    sinks.push_back(std::make_unique<FileSink>(file));
    sinks.push_back(std::make_unique<ConsoleSink>());

    std::ofstream clear(file, std::ios::trunc);
    logger = std::make_unique<Logger>(100, std::chrono::milliseconds(100),
                                      std::move(sinks));
  }

  void TearDown() override {
    std::filesystem::remove(file);
    std::cout.rdbuf(old_buf);
  }

  std::unique_ptr<Logger> logger;
  std::string file;
  std::ostringstream captured;
  std::streambuf *old_buf;
};

class BatchLoggerFixture : public ::testing::Test {
protected:
  void SetUp() override {
    file = "test_logs";
    old_buf = std::cout.rdbuf(captured.rdbuf());

    std::vector<std::unique_ptr<Sink>> sinks;
    sinks.push_back(std::make_unique<FileSink>(file));
    sinks.push_back(std::make_unique<ConsoleSink>());

    std::ofstream clear(file, std::ios::trunc);
    logger = std::make_unique<Logger>(5, std::chrono::milliseconds(5000),
                                      std::move(sinks));
  }

  void TearDown() override {
    std::filesystem::remove(file);
    std::cout.rdbuf(old_buf);
  }

  std::unique_ptr<Logger> logger;
  std::string file;
  std::ostringstream captured;
  std::streambuf *old_buf;
};

class WrongFileLoggerFixture : public ::testing::Test {
protected:
  void SetUp() override {
    file = "nonexisten_dir_xyz/test_logs";
    other_file = "nonexistent_dir_123/test_logs";
    old_buf = std::cout.rdbuf(captured.rdbuf());

    std::vector<std::unique_ptr<Sink>> sinks;
    sinks.push_back(std::make_unique<FileSink>(file));
    sinks.push_back(std::make_unique<ConsoleSink>());

    logger = std::make_unique<Logger>(5, std::chrono::milliseconds(5000),
                                      std::move(sinks));
  }

  void TearDown() override {
    std::filesystem::remove(file);
    std::cout.rdbuf(old_buf);
  }

  std::unique_ptr<Logger> logger;
  std::string file;
  std::string other_file;
  std::ostringstream captured;
  std::streambuf *old_buf;
};

class WrongFilesLoggerFixture : public ::testing::Test {
protected:
  void SetUp() override {
    file = "nonexisten_dir_xyz/test_logs";
    other_file = "nonexistent_dir_123/test_logs";
    old_buf = std::cout.rdbuf(captured.rdbuf());

    std::vector<std::unique_ptr<Sink>> sinks;
    sinks.push_back(std::make_unique<FileSink>(file));
    sinks.push_back(std::make_unique<FileSink>(file));
    sinks.push_back(std::make_unique<ConsoleSink>());

    logger = std::make_unique<Logger>(5, std::chrono::milliseconds(5000),
                                      std::move(sinks));
  }

  void TearDown() override {
    std::filesystem::remove(file);
    std::cout.rdbuf(old_buf);
  }

  std::unique_ptr<Logger> logger;
  std::string file;
  std::string other_file;
  std::ostringstream captured;
  std::streambuf *old_buf;
};

class MultiThresholdLoggerFixture : public ::testing::Test {
protected:
  void SetUp() override {
    file = "test_logs";
    other_file = "error.txt";
    old_cout_buf = std::cout.rdbuf(captured_cout.rdbuf());
    old_cerr_buf = std::cerr.rdbuf(captured_cerr.rdbuf());

    std::vector<std::unique_ptr<Sink>> sinks;
    sinks.push_back(std::make_unique<FileSink>(file, LogLevel::Info));
    sinks.push_back(std::make_unique<NotifierSink>(LogLevel::Error));
    sinks.push_back(std::make_unique<ConsoleSink>(LogLevel::Debug));

    std::ofstream clear(file, std::ios::trunc);
    logger = std::make_unique<Logger>(100, std::chrono::milliseconds(5000),
                                      std::move(sinks), LogLevel::Debug);
  }

  void TearDown() override {
    std::filesystem::remove(file);
    std::filesystem::remove(other_file);
    std::cout.rdbuf(old_cout_buf);
    std::cerr.rdbuf(old_cerr_buf);
  }

  std::unique_ptr<Logger> logger;
  std::string file;
  std::string other_file;
  std::ostringstream captured_cout, captured_cerr;
  std::streambuf *old_cout_buf, *old_cerr_buf;
};

std::string read_file(const std::string &filename = "test_logs") {
  std::ifstream file(filename);
  if (!file) {
    return "";
  }

  std::stringstream buffer;
  buffer << file.rdbuf();

  return buffer.str();
}

bool check_file(const std::string &expected,
                const std::string &filename = "test_logs",
                const std::chrono::milliseconds timeout = 5000ms) {

  std::string content;
  auto start = std::chrono::steady_clock::now();
  while (std::chrono::steady_clock::now() - start < timeout) {
    content = read_file(filename);
    if (content == expected) {
      return true;
    }
  }
  return false;
}

bool check_file(const std::unordered_multiset<std::string> &expected,
                const std::string &filename = "test_logs",
                const std::chrono::milliseconds timeout = 5000ms) {

  std::string content;
  auto start = std::chrono::steady_clock::now();
  while (std::chrono::steady_clock::now() - start < timeout) {
    std::unordered_multiset<std::string> actual;
    std::ifstream file("test_logs");
    std::string line;

    while (std::getline(file, line)) {
      actual.insert(line);
    }

    if (actual == expected) {
      return true;
    }
  }
  return false;
}

TEST_F(LoggerFixture, SingleWrite) {
  logger->log("Test");
  EXPECT_EQ(read_file(file), "");

  std::string output = captured.str();
  EXPECT_EQ(output, "");

  auto result = logger->flush();
  EXPECT_TRUE(result.has_value());

  EXPECT_EQ(read_file("test_logs"), "[INFO] Test\n");

  output = captured.str();
  EXPECT_EQ(output, "[INFO] Test\n");
}

TEST_F(LoggerFixture, MultipleWrites) {
  logger->log("Multi");
  logger->log("line");
  logger->log("log");
  EXPECT_EQ(read_file(file), "");

  std::string output = captured.str();
  EXPECT_EQ(output, "");

  auto result = logger->flush();
  EXPECT_TRUE(result.has_value());

  EXPECT_EQ(read_file("test_logs"), "[INFO] Multi\n[INFO] line\n[INFO] log\n");

  output = captured.str();
  EXPECT_EQ(output, "[INFO] Multi\n[INFO] line\n[INFO] log\n");
}

TEST_F(LoggerFixture, LoggerPersistence) {
  logger->log("before crash");
  logger.reset();
  EXPECT_EQ(read_file(file), "[INFO] before crash\n");

  std::string output = captured.str();
  EXPECT_EQ(output, "[INFO] before crash\n");

  std::vector<std::unique_ptr<Sink>> sinks;
  sinks.push_back(std::make_unique<FileSink>(file));
  sinks.push_back(std::make_unique<ConsoleSink>());

  Logger second_logger =
      Logger(1, std::chrono::milliseconds(1000), std::move(sinks));
  second_logger.log("after crash");
  auto result = second_logger.flush();
  EXPECT_TRUE(result.has_value());

  EXPECT_EQ(read_file("test_logs"),
            "[INFO] before crash\n[INFO] after crash\n");

  output = captured.str();
  EXPECT_EQ(output, "[INFO] before crash\n[INFO] after crash\n");
}

TEST_F(LoggerFixture, TwoOpenLogger) {
  logger->log("before crash");
  auto result = logger->flush();
  EXPECT_TRUE(result.has_value());

  std::vector<std::unique_ptr<Sink>> sinks;
  sinks.push_back(std::make_unique<FileSink>(file));
  sinks.push_back(std::make_unique<ConsoleSink>());

  Logger second_logger =
      Logger(1, std::chrono::milliseconds(1000), std::move(sinks));

  second_logger.log("after crash");
  auto second_result = second_logger.flush();
  EXPECT_TRUE(second_result.has_value());

  EXPECT_EQ(read_file("test_logs"),
            "[INFO] before crash\n[INFO] after crash\n");

  std::string output = captured.str();
  EXPECT_EQ(output, "[INFO] before crash\n[INFO] after crash\n");
}

void log(std::unique_ptr<Logger> &&logger) {
  logger->log("moved");
  auto result = logger->flush();
  EXPECT_TRUE(result.has_value());
}

TEST_F(LoggerFixture, LogThroughMovedLogger) {
  log(std::move(logger));

  EXPECT_EQ(read_file("test_logs"), "[INFO] moved\n");
}

TEST(LoggerTraits, CopyIsDisabled) {
  static_assert(!std::is_copy_constructible_v<Logger>);
  static_assert(!std::is_copy_assignable_v<Logger>);
}

TEST(LoggerTraits, MoveIsDisabled) {
  static_assert(!std::is_move_constructible_v<Logger>);
  static_assert(!std::is_move_assignable_v<Logger>);
}

TEST_F(LoggerFixture, EmptyString) {
  logger->log("");
  auto result = logger->flush();
  EXPECT_TRUE(result.has_value());

  EXPECT_EQ(read_file("test_logs"), "[INFO] \n");
}

TEST_F(IntervalLoggerFixture, IntervalFlush) {
  logger->log("Hello World!");

  EXPECT_TRUE(check_file("[INFO] Hello World!\n"));
}

TEST_F(IntervalLoggerFixture, IntervalLoggerPersistence) {
  logger->log("before crash");
  logger.reset();

  std::vector<std::unique_ptr<Sink>> sinks;
  sinks.push_back(std::make_unique<FileSink>(file));

  Logger second_logger =
      Logger(1, std::chrono::milliseconds(1000), std::move(sinks));

  second_logger.log("after crash");

  EXPECT_TRUE(check_file("[INFO] before crash\n[INFO] after crash\n"));
}

TEST_F(LoggerFixture, FlushOnDestruction) {
  logger->log("Hello");
  logger.reset();

  EXPECT_EQ(read_file(file), "[INFO] Hello\n");
}

TEST_F(BatchLoggerFixture, FlushOnBatchSize) {
  for (size_t i = 0; i < 5; i++) {
    logger->log("Hello");
  }

  std::this_thread::sleep_for(50ms);

  EXPECT_EQ(
      read_file(file),
      "[INFO] Hello\n[INFO] Hello\n[INFO] Hello\n[INFO] Hello\n[INFO] Hello\n");
}

TEST_F(IntervalLoggerFixture, Concurency) {
  std::vector<std::thread> threads;
  std::unordered_multiset<std::string> expected;
  std::mutex mtx;
  for (size_t i = 0; i < 4; i++) {
    threads.push_back(std::thread([this, i, &expected, &mtx]() {
      for (size_t j = 0; j < 30; j++) {
        std::string line = std::format("Thread {}, log {}", i, j);
        logger->log(line);
        {
          std::unique_lock<std::mutex> lock(mtx);
          expected.insert("[INFO] " + line);
        }
      }
    }));
  }

  for (auto &t : threads) {
    t.join();
  }

  EXPECT_TRUE(check_file(expected));
}

TEST_F(WrongFileLoggerFixture, FileNotOpenReturnsError) {
  logger->log("failure");
  auto result = logger->flush();
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), std::vector({LogError::FileNotOpen}));
  EXPECT_EQ(captured.str(), "[INFO] failure\n");
}

TEST_F(WrongFilesLoggerFixture, ReturnsMultipleErrors) {
  logger->log("failure");
  auto result = logger->flush();
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(),
            std::vector({LogError::FileNotOpen, LogError::FileNotOpen}));
  EXPECT_EQ(captured.str(), "[INFO] failure\n");
}

TEST_F(LoggerFixture, LogsAtOrAboveLevel) {
  logger->log("should log", LogLevel::Info);
  logger->log("Should not log", LogLevel::Debug);
  logger->log("should log", LogLevel::Warning);

  auto result = logger->flush();
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(read_file(file), "[INFO] should log\n[WARNING] should log\n");
}

TEST_F(LoggerFixture, LogLevelChangesCorrectly) {
  logger->set_log_level(LogLevel::Error);
  logger->log("should not log", LogLevel::Info);
  logger->log("should log", LogLevel::Error);

  auto result = logger->flush();
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(read_file(file), "[ERROR] should log\n");
}

TEST_F(MultiThresholdLoggerFixture, MultiThresholdLogs) {
  logger->log("debug", LogLevel::Debug);
  logger->log("info", LogLevel::Info);
  logger->log("error", LogLevel::Error);

  auto result = logger->flush();
  EXPECT_TRUE(result.has_value());

  EXPECT_EQ(read_file(file), "[INFO] info\n[ERROR] error\n");
  EXPECT_EQ(captured_cerr.str(), "[ERROR] error\n");
  EXPECT_EQ(captured_cout.str(), "[DEBUG] debug\n[INFO] info\n[ERROR] error\n");
}
