#include "cli_app.h"

#include <csignal>

#include "app_controller.h"
#include "system_params.h"

CliApp::CliApp(const cmdline::parser &parser)
{
    json_config_path_ = parser.get<std::string>("config");
    method_ = static_cast<Method>(parser.get<int>("method") - 1);

    handle_map_["log"] = [this](const std::any &event)
    {
        if (auto msg = std::any_cast<Logger::LogMessage>(&event))
        {
            std::cout << msg->message << std::endl;
        }
        else
        {
            std::cout << "Type mismatch." << std::endl;
        }
    };

    handle_map_["exit"] = [this](const std::any &event)
    {
        if (auto msg = std::any_cast<bool>(&event))
        {
            running_.store(*msg);
            running_.notify_all();
        }
        else
        {
            std::cout << "Type mismatch." << std::endl;
        }
    };

    method_map_[Method::Calibrating] = [this](const std::string &command)
    {
        AppController::instance().publish("task_command", std::make_any<std::pair<std::string, std::string>>("calibration", command));
    };
    method_map_[Method::Reconstructing] = [this](const std::string &command)
    {
        AppController::instance().publish("task_command", std::make_any<std::pair<std::string, std::string>>("reconstruction", command));
    };
    method_map_[Method::Merging] = [this](const std::string &command)
    {
        AppController::instance().publish("task_command", std::make_any<std::pair<std::string, std::string>>("merger", command));
    };

    std::thread([this]
                {
        char ch = std::cin.get();
        if (ch == '\n')
        {handle_map_["exit"](false);} })
        .detach();
}

CliApp::~CliApp()
{
}

void CliApp::subscribe(const std::string &topic)
{
    AppController::instance().registerSubscribe(topic, shared_from_this());
}

void CliApp::unsubscribe(const std::string &topic)
{
    AppController::instance().cancelSubscribe(topic, shared_from_this());
}

void CliApp::handleEvent(const std::string &topic, const std::any &event)
{
    auto it = handle_map_.find(topic);
    if (it != handle_map_.end())
    {
        it->second(event);
    }
    else
    {
        AppController::instance().getLogger().log("No handler for topic: " + topic);
    }
}

void CliApp::run()
{
    this->running_.store(true);
    this->subscribe("log");
    this->subscribe("progress");
    this->subscribe("exit");

    SystemParams::init(json_config_path_, Mode::Term);
    std::cout << static_cast<int>(method_) << std::endl;
    this->method_map_[method_]("start");

    running_.wait(true);
    AppController::instance().stop();
}