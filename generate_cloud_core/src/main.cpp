#include <fstream>
#include <iostream>

#include "cmdline.h"
#include "nlohmann/json.hpp"

#include "system_params.h"
#include "task_manager_impl.h"

#include "calibration_task.h"
#include "reconstruction_task.h"

void runUiMode()
{
    Logger::instance().log("[UI] Run in UI mode...");
}

void runCliMode(const cmdline::parser &parser)
{
    Logger::instance().log("[CLI] Run in CLI mode...");

    std::string json_config_path = parser.get<std::string>("json-config");

    TaskManager &task_manager = TaskManagerImpl::instance();
    task_manager.start();

    SystemParams::init(json_config_path, RunMode::Cli);

    if (SystemParams::instance().calibration_flag_)
    {
        auto calibration_task = new CalibrationTask(3);
        task_manager.startTask(calibration_task->getId(), calibration_task);
    }

    auto reconstruction_task = new ReconstructionTask(2);
    task_manager.startTask(reconstruction_task->getId(), reconstruction_task);

    task_manager.stop();
}

int main(int argc, char *argv[])
{
    cmdline::parser parser;
    parser.add("ui", 'u', "Run in UI mode");
    parser.add("cli", 'c', "Run in CLI mode");
    parser.add<std::string>("json-config", 'j', "Config path", false, "config.json");

    parser.parse_check(argc, argv);

    if (parser.exist("ui"))
    {
        Logger::instance().setMode(LogMode::Ui);
        runUiMode();
    }
    else
    {
        Logger::instance().setMode(LogMode::Cli);
        runCliMode(parser);
    }

    return 0;
}