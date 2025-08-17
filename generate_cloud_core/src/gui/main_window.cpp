#include "main_window.h"

MainWindow::MainWindow()
{
    log_panel_ = std::make_shared<LogPanel>();
    params_panel_ = std::make_shared<ParamsPanel>();
    // task_panel_ = std::make_unique<TaskPanel>();

    AppController::instance().registerSubscribe("log", log_panel_);

    AppController::instance().registerSubscribe("params_init_finished", params_panel_);
    AppController::instance().registerSubscribe("params_update", params_panel_);
}

MainWindow::~MainWindow() = default;

void MainWindow::render()
{
    log_panel_->render();
    params_panel_->render();
    // task_panel_->render();
}
