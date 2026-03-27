#pragma once

#include <memory>
#include <vector>

#include "imgui.h"
#include "app_controller.h"

class LogPanel : public SubscriberAdapter
{
public:
    LogPanel();
    void render();

    void handleEvent(const std::string &topic, const std::any &event) override;

private:
    std::vector<Logger::LogMessage> logs_;
};
