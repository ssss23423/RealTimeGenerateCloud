#pragma once

#include <iostream>
#include <string>
#include <vector>

#include "system_params.h"

#define NONE_COLOR "\033[0m"
#define YELLOW "\033[1;33m"
#define RED "\033[0;31m"

class Logger
{
public:
    enum class LogLevel
    {
        Info,
        Warn,
        Error
    };

    struct LogMessage
    {
        LogLevel level;
        std::string message;
    };

    explicit Logger(const Mode &mode) : mode_(mode) {}

    void log(const std::string &message, LogLevel level = LogLevel::Info);

private:
    Mode mode_;
};
