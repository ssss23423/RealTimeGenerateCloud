#pragma once

#include <any>
#include <condition_variable>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <shared_mutex>
#include <string>

#include "subscriber.h"
#include "logger.h"
#include "task_manager.h"
#include "system_params.h"

class AppController
{
public:
    static AppController &instance();

    void init(Mode mode);
    Logger &getLogger();

    void start();
    void stop();

    void publish(const std::string &topic, std::any data);
    void registerSubscribe(const std::string &topic, std::shared_ptr<Subscriber> subscriber);
    void cancelSubscribe(const std::string &topic, std::shared_ptr<Subscriber> subscriber);

    static void signalHandler(int);

private:
    AppController() = default;
    AppController(const AppController &) = delete;
    AppController &operator=(const AppController &) = delete;
    virtual ~AppController() { this->stop(); }

    void coreProcess();

    Mode mode_;

    std::unique_ptr<Logger> logger_;
    std::shared_ptr<TaskManager> task_manager_;

    std::unordered_map<std::string, std::list<std::any>> publishers_;
    std::unordered_map<std::string, std::list<std::weak_ptr<Subscriber>>> subscribers_;

    std::thread core_process_;
    std::condition_variable cv_;
    std::mutex publish_mutex;
    std::shared_mutex subscribe_mutex;

    static inline std::atomic<bool> running_{false};
};
