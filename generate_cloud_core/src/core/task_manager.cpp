#include "task_manager.h"

#include "app_controller.h"
#include "calibration_task.h"
#include "reconstruction_task.h"

TaskManager::TaskManager(Mode mode)
{
    this->mode_ = mode;

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

    this->handle_map_["task_command"] = [this](const std::any &event)
    {
        if (auto msg = std::any_cast<std::pair<std::string, std::string>>(&event))
        {
            std::string task_id = msg->first;
            std::string command = msg->second;
            this->method_map_[task_id][command]();
        }
        else
        {
            std::cout << "Type mismatch." << std::endl;
        }
    };
}

void TaskManager::run()
{
    AppController::instance().registerSubscribe("task_command", shared_from_this());
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
