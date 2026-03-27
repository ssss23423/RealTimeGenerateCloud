#pragma once

#include <string>
#include <memory>

#include "imgui.h"
#include "cmdline.h"

#include "app_controller.h"
#include "main_window.h"

struct GLFWwindow;

class App : public Subscriber, public std::enable_shared_from_this<App>
{
public:
    App(const cmdline::parser &parser, int window_width = 1280, int window_height = 720, std::string title = "GenerateCloud");
    ~App();

    void run();

private:
    void init();
    void cleanup();
    void render();

    void subscribe(const std::string &topic) override;
    void unsubscribe(const std::string &topic) override;
    void handleEvent(const std::string &topic, const std::any &event) override; // only for log

    GLFWwindow *window_ = nullptr;
    std::unique_ptr<MainWindow> main_window_;

    ImGuiIO *io_ = nullptr;

    int window_width_;
    int window_height_;
    std::string title_;
    ImVec4 clear_color_ = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
};
