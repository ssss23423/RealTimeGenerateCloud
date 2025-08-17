#pragma once

#include <mutex>
#include <unordered_map>

#include "params_event.h"
#include "subscriber.h"
#include "task.h"
#include "system_params.h"

class TaskManager : public Subscriber, public std::enable_shared_from_this<TaskManager>
{
public:
    TaskManager(Mode mode);
    void run();
    void start();
    void stop();

    void startTask(std::string id, Task *task);
    void stopTask(std::string id);
    void stopAllTask();

    void subscribe(const std::string &topic) override;
    void unsubscribe(const std::string &topic) override;
    void handleEvent(const std::string &topic, const std::any &event) override;

private:
    Mode mode_;
    std::thread worker_;
    std::atomic<bool> running_{false};
    std::mutex mutex_;

    std::unordered_map<std::string, Task *> tasks_;
    std::unordered_map<std::string, std::unordered_map<std::string, std::function<void()>>> method_map_;
    std::unordered_map<std::string, std::function<void(const std::any &)>> handle_map_;
    std::unordered_map<std::string, std::function<void(const ParamUpdateEvent &)>> params_update_map_;

    void initMethodMap();
    void initHandleMap();
    void initParamsUpdateMap();

    void taskCmdHandler(const std::pair<std::string, std::string> &msg);
    void paramsInit(const std::pair<std::string, std::string> &event);
    void paramsReload(const std::pair<std::string, std::string> &event);
    void paramsUpdate(const ParamUpdateEvent &e);
};