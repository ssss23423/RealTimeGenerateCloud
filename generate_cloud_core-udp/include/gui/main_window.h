#pragma once

#include <memory>

#include "imgui.h"
#include "app_controller.h"
#include "log_panel.h"
#include "params_panel.h"
#include "task_panel.h"

class MainWindow
{
public:
    MainWindow();
    ~MainWindow();

    void render();

private:
    std::shared_ptr<LogPanel> log_panel_ = nullptr;
    std::shared_ptr<ParamsPanel> params_panel_ = nullptr;
    std::shared_ptr<TaskPanel> task_panel_ = nullptr;
};