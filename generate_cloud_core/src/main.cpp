#include <fstream>
#include <iostream>

#include "cmdline.h"
#include "nlohmann/json.hpp"

#include "system_params.h"
#include "app_controller.h"
#include "app.h"
#include "cli_app.h"

#ifdef _WIN32
//const std::string default_config_path = "D:/code/RealTimeGenerateCloud/config.json";
const std::string default_config_path = "D:/jiguang_3D/data_tran_3D/RealTimeGenerateCloud/config.json";
#else
const std::string default_config_path = "/data/project/GenerateCloud/config.json";
#endif

#define DEBUG

void runUiMode(const cmdline::parser &parser)
{
    auto app = std::make_shared<App>(parser);
    app->run();
}

void runCliMode(const cmdline::parser &parser)
{
    auto cli_app = std::make_shared<CliApp>(parser);
    cli_app->run();
}

int main(int argc, char *argv[])
{
#ifndef DEBUG
    cmdline::parser parser;
    parser.add("ui", 'u', "Run in UI mode");
    parser.add("terminal", 't', "Run in CLI mode");
    parser.add<std::string>("config", 'c', "Config path", false, default_config_path);
    parser.add<int>("method", 'm', "Process mode. [1] System calibration; [2] Cloud reconstructing; [3] Cloud merging", true);
    parser.parse_check(argc, argv);
    if (parser.exist("ui"))
    {
        AppController::instance().init(Mode::Ui);
        AppController::instance().start();
        runUiMode();
    }
    else
    {
        AppController::instance().init(Mode::Term);
        AppController::instance().start();
        runCliMode(parser);
    }
#else
    cmdline::parser parser;
    parser.add("ui", 'u', "Run in UI mode");
    parser.add("terminal", 't', "Run in CLI mode");
    parser.add<std::string>("config", 'c', "Config path", false, default_config_path);
    parser.parse_check(argc, argv);
    AppController::instance().init(Mode::Ui);
    AppController::instance().start();
    runUiMode(parser);
#endif

    return 0;
}