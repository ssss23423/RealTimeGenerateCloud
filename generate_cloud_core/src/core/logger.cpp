#include "logger.h"

Logger &Logger::instance()
{
    static Logger instance;
    return instance;
}

void Logger::log(const std::string &message, LogLevel level)
{
    std::string level_str;
    if (mode_ == LogMode::Ui)
    {
        switch (level)
        {
        case LogLevel::Info:
            level_str = "[INFO] ";
            break;
        case LogLevel::Warn:
            level_str = "[WARN] ";
            break;
        case LogLevel::Error:
            level_str = "[ERROR] ";
            break;
        }
    }
    else
    {
        switch (level)
        {
        case LogLevel::Info:
            level_str = NONE_COLOR "[INFO] ";
            break;
        case LogLevel::Warn:
            level_str = YELLOW "[WARN] ";
            break;
        case LogLevel::Error:
            level_str = RED "[ERROR] ";
            break;
        }
    }

    std::string full_message = level_str + message;
    std::cout << full_message << std::endl;

    for (auto &cb : callbacks_)
    {
        cb({level, full_message});
    }
}

void Logger::registerCallback(LogCallback cb)
{
    callbacks_.push_back(cb);
}

void Logger::setMode(LogMode mode)
{
    mode_ = mode;
}
