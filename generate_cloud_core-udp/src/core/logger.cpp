#include "logger.h"
#include "app_controller.h"

void Logger::log(const std::string &message, LogLevel level)
{
    std::string level_str;

    static const std::array<const char *, 3> ui_prefix = {"[INFO] ", "[WARN] ", "[ERROR] "};
    static const std::array<const char *, 3> term_prefix = {NONE_COLOR "[INFO] ", YELLOW "[WARN] ", RED "[ERROR] "};

    const auto &prefix = (mode_ == Mode::Ui) ? ui_prefix : term_prefix;
    std::string full_message = prefix[static_cast<size_t>(level)] + message;

    AppController::instance().publish("log", std::make_any<LogMessage>(level, full_message));
}
