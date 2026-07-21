#include "logger.h"
#include "sinks.h"
#include <chrono>
#include <fstream>
#include <memory>

int main() {
  std::string file = "logs.txt";
  std::ofstream clear(file, std::ios::trunc);

  std::vector<std::unique_ptr<Sink>> sinks;
  sinks.push_back(std::make_unique<FileSink>(file));
  sinks.push_back(std::make_unique<ConsoleSink>());
  std::chrono::milliseconds interval(1000);

  Logger logger = Logger(30, interval, std::move(sinks));
  logger.log("Hello");
  logger.log("world!");
  logger.log("This is the logger");
}
