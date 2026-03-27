#include "task_manager.h"

#include "app_controller.h"
#include "calibration_task.h"
#include "reconstruction_task.h"
#include "real_time_reconstruction_task.h"

TaskManager::TaskManager(Mode mode)
{
    this->mode_ = mode;
    this->initMethodMap();
    this->initHandleMap();
    this->initParamsUpdateMap();
}

void TaskManager::run()
{
    while (running_.load())
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);

            for (auto it = tasks_.begin(); it != tasks_.end();)
            {
                auto &[id, task] = *it;
                if (task)
                {
                    if (!task->isFinished())
                    {
                        if (!task->isRunning())
                        {
                            task->start();
                            AppController::instance().getLogger().log("Task " + id + " started.");
                        }
                        ++it;
                    }
                    else
                    {
                        AppController::instance().getLogger().log("Task " + id + " finished and removed.");
                        it = tasks_.erase(it);
                    }
                }
                else
                {
                    ++it;
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void TaskManager::start()
{
    AppController::instance().registerSubscribe("task_command", shared_from_this());
    AppController::instance().registerSubscribe("params_init", shared_from_this());
    AppController::instance().registerSubscribe("params_reload", shared_from_this());
    AppController::instance().registerSubscribe("params_update", shared_from_this());

    if (!running_.load())
    {
        running_.store(true);
        worker_ = std::thread(&TaskManager::run, this);
    }
}

void TaskManager::stop()
{
    this->stopAllTask();

    running_.store(false);
    if (worker_.joinable())
    {
        worker_.join();
    }
}

void TaskManager::startTask(std::string id, Task *task)
{
    Task *oldTask = nullptr;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = tasks_.find(id);
        if (it != tasks_.end())
        {
            AppController::instance().getLogger().log("Task " + id + " already exists. Stopping old task.", Logger::LogLevel::Warn);
            oldTask = it->second;
            tasks_.erase(it);
        }
    }

    if (oldTask)
    {
        oldTask->stop();
        delete oldTask;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        tasks_[id] = task;
    }

    AppController::instance().getLogger().log("Task " + id + " is starting...");
    task->start();
}

void TaskManager::stopTask(std::string id)
{
    Task *taskToStop = nullptr;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = tasks_.find(id);
        if (it != tasks_.end())
        {
            taskToStop = it->second;
            tasks_.erase(it);
        }
    }

    if (taskToStop)
    {
        AppController::instance().getLogger().log("Stopping Task " + id);
        taskToStop->stop();
    }
}

void TaskManager::stopAllTask()
{
    std::lock_guard<std::mutex> lock(mutex_);

    for (auto &[id, task] : tasks_)
    {
        if (task)
        {
            task->stop();
            delete task;
        }
    }

    tasks_.clear();
    std::string msg = "All tasks stopped.";
    AppController::instance().getLogger().log(msg);
}

void TaskManager::subscribe(const std::string &topic)
{
    AppController::instance().registerSubscribe(topic, shared_from_this());
}

void TaskManager::unsubscribe(const std::string &topic)
{
    AppController::instance().cancelSubscribe(topic, shared_from_this());
}

void TaskManager::handleEvent(const std::string &topic, const std::any &event)
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

void TaskManager::initMethodMap()
{
    this->method_map_["calibration"]["start"] = [this]()
    {
        auto calibration_task = new CalibrationTask("calibration");
        this->startTask("calibration", calibration_task);
    };
    this->method_map_["calibration"]["stop"] = [this]()
    {
        this->stopTask("calibration");
    };

    this->method_map_["reconstruction"]["start"] = [this]()
    {
        auto reconstruction_task = new ReconstructionTask("reconstruction");
        this->startTask("reconstruction", reconstruction_task);
    };
    this->method_map_["reconstruction"]["stop"] = [this]()
    {
        this->stopTask("reconstruction");
    };

    // this->method_map_["merger"]["start"] = [this]()
    // {
    //     auto calibration_task = new CalibrationTask("merger");
    //     this->startTask("merger", calibration_task);
    // };
    // this->method_map_["merger"]["stop"] = [this]()
    // {
    //     this->stopTask("merger");
    // };

    //new task : real time reconstruction
    this->method_map_["real_time_reconstruction"]["start"] = [this]()
    {
        auto rt_task = new RealTimeReconstructionTask("real_time_reconstruction");
        this->startTask("real_time_reconstruction", rt_task);
    };

    this->method_map_["real_time_reconstruction"]["stop"] = [this]()
    {
        this->stopTask("real_time_reconstruction");
    };
}

void TaskManager::initHandleMap()
{
    this->handle_map_["task_command"] = [this](const std::any &event)
    {
        if (auto msg = std::any_cast<std::pair<std::string, std::string>>(&event))
        {
            this->taskCmdHandler(*msg);
        }
        else
        {
            std::cout << "Type mismatch." << std::endl;
        }
    };

    this->handle_map_["params_init"] = [this](const std::any &event)
    {
        if (auto e = std::any_cast<std::pair<std::string, std::string>>(&event))
        {
            this->paramsInit(*e);
        }
        else
        {
            std::cout << "Type mismatch." << std::endl;
        }
    };

    this->handle_map_["params_reload"] = [this](const std::any &event)
    {
        if (auto e = std::any_cast<std::pair<std::string, std::string>>(&event))
        {
            this->paramsReload(*e);
        }
        else
        {
            std::cout << "Type mismatch." << std::endl;
        }
    };

    this->handle_map_["params_update"] = [this](const std::any &event)
    {
        if (auto e = std::any_cast<ParamUpdateEvent>(&event))
        {
            if (e->origin == "ui")
            {
                this->paramsUpdate(*e);
            }
        }
    };
}

void TaskManager::initParamsUpdateMap()
{
    this->params_update_map_["poses_dir"] = [this](const ParamUpdateEvent &event)
    {
        try
        {
            std::string val = std::any_cast<const std::string &>(event.value);
            auto full_path = std::filesystem::weakly_canonical(SystemParams::instance().getConfigBaseDir() / val);
            SystemParams::instance().hv_program_params_.hv_poses_dir = full_path.c_str();
            SystemParams::instance().validatePaths(Path::PosesDir);
        }
        catch (const std::exception &e)
        {
            std::string msg = "Error in updating params: " + std::string(e.what());
            AppController::instance().getLogger().log(msg, Logger::LogLevel::Error);
        }
    };

    this->params_update_map_["calibration_images_dir"] = [this](const ParamUpdateEvent &event)
    {
        try
        {
            std::string val = std::any_cast<const std::string &>(event.value);
            auto full_path = std::filesystem::weakly_canonical(SystemParams::instance().getConfigBaseDir() / val);
            SystemParams::instance().hv_program_params_.hv_calibration_images_dir = full_path.c_str();
            SystemParams::instance().validatePaths(Path::CalibrationImageDir);
        }
        catch (const std::exception &e)
        {
            std::string msg = "Error in updating params: " + std::string(e.what());
            AppController::instance().getLogger().log(msg, Logger::LogLevel::Error);
        }
    };

    this->params_update_map_["caltab_description"] = [this](const ParamUpdateEvent &event)
    {
        try
        {
            std::string val = std::any_cast<const std::string &>(event.value);
            auto full_path = std::filesystem::weakly_canonical(SystemParams::instance().getConfigBaseDir() / val);
            SystemParams::instance().hv_program_params_.hv_caltab_description_path = full_path.c_str();
            SystemParams::instance().validatePaths(Path::CaltabDescription);
        }
        catch (const std::exception &e)
        {
            std::string msg = "Error in updating params: " + std::string(e.what());
            AppController::instance().getLogger().log(msg, Logger::LogLevel::Error);
        }
    };

    this->params_update_map_["calibration_image_count"] = [this](const ParamUpdateEvent &event)
    {
        int val = std::any_cast<int>(event.value);
        SystemParams::instance().hv_program_params_.hv_calibration_image_count = val;
    };

    this->params_update_map_["calibration_min_threshold"] = [this](const ParamUpdateEvent &event)
    {
        int val = std::any_cast<int>(event.value);
        SystemParams::instance().hv_program_params_.hv_calibration_min_threshold = val;
    };

    this->params_update_map_["calibration_step_count"] = [this](const ParamUpdateEvent &event)
    {
        int val = std::any_cast<int>(event.value);
        SystemParams::instance().hv_program_params_.hv_calibration_step_count = val;
    };

    this->params_update_map_["calibration_thickness"] = [this](const ParamUpdateEvent &event)
    {
        float val = std::any_cast<float>(event.value);
        SystemParams::instance().hv_program_params_.hv_calibration_thickness = val;
    };

    this->params_update_map_["calibration_max_light_plane_error"] = [this](const ParamUpdateEvent &event)
    {
        float val = std::any_cast<float>(event.value);
        SystemParams::instance().hv_program_params_.hv_calibration_max_light_plane_error = val;
    };

    this->params_update_map_["reconstruction_image_data"] = [this](const ParamUpdateEvent &event)
    {
        try
        {
            std::string val = std::any_cast<const std::string &>(event.value);
            auto full_path = std::filesystem::weakly_canonical(SystemParams::instance().getConfigBaseDir() / val);
            SystemParams::instance().hv_program_params_.hv_reconstruction_image_data_path = full_path.c_str();
            SystemParams::instance().validatePaths(Path::ReconstructionImageDir);
        }
        catch (const std::exception &e)
        {
            std::string msg = "Error in updating params: " + std::string(e.what());
            AppController::instance().getLogger().log(msg, Logger::LogLevel::Error);
        }
    };

    this->params_update_map_["reconstruction_output_cloud_dir"] = [this](const ParamUpdateEvent &event)
    {
        try
        {
            std::string val = std::any_cast<const std::string &>(event.value);
            auto full_path = std::filesystem::weakly_canonical(SystemParams::instance().getConfigBaseDir() / val);
            SystemParams::instance().hv_program_params_.hv_reconstruction_output_dir = full_path.c_str();
            SystemParams::instance().validatePaths(Path::ReconstructionOutputDir);
        }
        catch (const std::exception &e)
        {
            std::string msg = "Error in updating params: " + std::string(e.what());
            AppController::instance().getLogger().log(msg, Logger::LogLevel::Error);
        }
    };

    this->params_update_map_["reconstruction_min_threshold"] = [this](const ParamUpdateEvent &event)
    {
        int val = std::any_cast<int>(event.value);
        SystemParams::instance().hv_program_params_.hv_reconstruction_min_threshold = val;
    };

    this->params_update_map_["reconstruction_roi_row1"] = [this](const ParamUpdateEvent &event)
    {
        int val = std::any_cast<int>(event.value);
        SystemParams::instance().hv_program_params_.hv_reconstruction_roi_row1 = val;
    };

    this->params_update_map_["reconstruction_roi_col1"] = [this](const ParamUpdateEvent &event)
    {
        int val = std::any_cast<int>(event.value);
        SystemParams::instance().hv_program_params_.hv_reconstruction_roi_col1 = val;
    };

    this->params_update_map_["reconstruction_roi_row2"] = [this](const ParamUpdateEvent &event)
    {
        int val = std::any_cast<int>(event.value);
        SystemParams::instance().hv_program_params_.hv_reconstruction_roi_row2 = val;
    };

    this->params_update_map_["reconstruction_roi_col2"] = [this](const ParamUpdateEvent &event)
    {
        int val = std::any_cast<int>(event.value);
        SystemParams::instance().hv_program_params_.hv_reconstruction_roi_col2 = val;
    };

    this->params_update_map_["save_cloud_flag"] = [this](const ParamUpdateEvent &event)
    {
        bool val = std::any_cast<bool>(event.value);
        SystemParams::instance().save_cloud_flag_ = val;
    };
    //2026.1.18 add 
    this->params_update_map_["save_line_cloud_flag"] = [this](const ParamUpdateEvent &event)
    {
        bool val = std::any_cast<bool>(event.value);
        SystemParams::instance().save_line_cloud_flag_ = val;
    };
    this->params_update_map_["line_cloud_stride"] = [this](const ParamUpdateEvent &event)
    {
        int val = std::any_cast<int>(event.value);
        if (val < 1) val = 1;
        SystemParams::instance().line_cloud_stride_ = val;
    };
    this->params_update_map_["line_cloud_file_type"] = [this](const ParamUpdateEvent &event)
    {
        std::string val = std::any_cast<const std::string &>(event.value);
        SystemParams::instance().line_cloud_file_type_ = val;
    };
    //2026.1.18 add finish

    //2026.3.1 add udp
    this->params_update_map_["udp_send_enabled"] = [this](const ParamUpdateEvent &event)
    {
        bool val = std::any_cast<bool>(event.value);
        SystemParams::instance().udp_send_enabled_ = val;
    };

    this->params_update_map_["udp_host"] = [this](const ParamUpdateEvent &event)
    {
        std::string val = std::any_cast<const std::string &>(event.value);
        SystemParams::instance().udp_host_ = val;
    };

    this->params_update_map_["udp_port"] = [this](const ParamUpdateEvent &event)
    {
        int val = std::any_cast<int>(event.value);
        if (val < 1) val = 1;
        if (val > 65535) val = 65535;
        SystemParams::instance().udp_port_ = val;
    };

    this->params_update_map_["udp_mtu"] = [this](const ParamUpdateEvent &event)
    {
        int val = std::any_cast<int>(event.value);
        if (val < 256) val = 256;
        if (val > 65000) val = 65000;
        SystemParams::instance().udp_mtu_ = val;
    };
    //2026.3.1 add finish

    this->params_update_map_["set_roi_flag"] = [this](const ParamUpdateEvent &event)
    {
        bool val = std::any_cast<bool>(event.value);
        SystemParams::instance().set_roi_flag_ = val;
    };

    this->params_update_map_["select_entire_frame"] = [this](const ParamUpdateEvent &event)
    {
        bool val = std::any_cast<bool>(event.value);
        SystemParams::instance().select_entire_frame_ = val;
    };
}

void TaskManager::taskCmdHandler(const std::pair<std::string, std::string> &msg)
{
    std::string task_id = msg.first;
    std::string command = msg.second;

    auto task_it = this->method_map_.find(task_id);
    if (task_it != this->method_map_.end())
    {
        auto cmd_it = task_it->second.find(command);
        if (cmd_it != task_it->second.end())
        {
            cmd_it->second();
        }
        else
        {
            AppController::instance().getLogger().log("Command not found for task: " + task_id, Logger::LogLevel::Warn);
        }
    }
    else
    {
        AppController::instance().getLogger().log("Task ID not found: " + task_id, Logger::LogLevel::Warn);
    }
}

void TaskManager::paramsInit(const std::pair<std::string, std::string> &event)
{
    std::string config_path = event.first;
    std::string mode = event.second;

    if (mode == "gui")
    {
        SystemParams::init(config_path, Mode::Ui);
    }
    else if (mode == "term")
    {
        SystemParams::init(config_path, Mode::Term);
    }
    else
    {
        AppController::instance().getLogger().log("Invalid mode: " + mode, Logger::LogLevel::Warn);
    }
}

void TaskManager::paramsReload(const std::pair<std::string, std::string> &event)
{
    std::string config_path = event.first;
    std::string mode = event.second;

    if (mode == "gui")
    {
        SystemParams::instance().reload(config_path, Mode::Ui);
    }
    else if (mode == "term")
    {
        SystemParams::instance().reload(config_path, Mode::Term);
    }
    else
    {
        AppController::instance().getLogger().log("Invalid mode: " + mode, Logger::LogLevel::Warn);
    }
}

void TaskManager::paramsUpdate(const ParamUpdateEvent &e)
{
    if (auto it = params_update_map_.find(e.name); it != params_update_map_.end())
    {
        it->second(e);
    }
    else
    {
        std::cerr << "Unknown param: " << e.name << std::endl;
    }
}