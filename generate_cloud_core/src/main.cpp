#include <fstream>
#include <iostream>

#include "cmdline.h"
#include "nlohmann/json.hpp"

#include "system_params.h"
#include "app_controller.h"

#include "cli_app.h"

void runUiMode()
{
    AppController::instance().getLogger().log("Run in UI mode...");
}

void runCliMode(const cmdline::parser &parser)
{
    auto cli_app = std::make_shared<CliApp>(parser);
    cli_app->run();
}

int main(int argc, char *argv[])
{
    cmdline::parser parser;
    parser.add("ui", 'u', "Run in UI mode");
    parser.add("terminal", 't', "Run in CLI mode");
    parser.add<std::string>("config", 'c', "Config path", false, "/data/project/GenerateCloud/config.json");
    // parser.add<int>("method", 'm', "Process mode. [1] System calibration; [2] Cloud reconstructing; [3] Cloud merging", true);
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

    AppController::instance().stop();
    return 0;
}