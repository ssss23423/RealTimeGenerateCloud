#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <functional>

#define NONE_COLOR "\033[0m"
#define YELLOW "\033[1;33m"
#define RED "\033[0;31m"

enum class LogLevel
{
    Info,
    Warn,
    Error
};

enum class LogMode
{
    Ui,
    Cli
};

struct LogMessage
{
    LogLevel level;
    std::string message;
};

class Logger
{
public:
    using LogCallback = std::function<void(const LogMessage &)>;

    static Logger &instance();
    void log(const std::string &message, LogLevel level = LogLevel::Info);
    void registerCallback(LogCallback cb);

    void setMode(LogMode mode);

private:
    Logger() = default;
    Logger(const Logger &) = delete;
    Logger &operator=(const Logger &) = delete;

    LogMode mode_;
    std::vector<LogCallback> callbacks_;
};
