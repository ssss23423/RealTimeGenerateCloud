#include "app_controller.h"

#include <csignal>

AppController &AppController::instance()
{
    static AppController inst;
    return inst;
}

void AppController::init(Mode mode)
{
    this->mode_ = mode;
    this->logger_ = std::make_unique<Logger>(mode);
    this->task_manager_ = std::make_shared<TaskManager>(mode);

    std::signal(SIGINT, AppController::signalHandler);
}

Logger &AppController::getLogger()
{
    return *logger_;
}

void AppController::start()
{
    running_.store(true);
    this->task_manager_->start();
    this->core_process_ = std::thread([this]()
                                      { this->coreProcess(); });
}

void AppController::stop()
{
    {
        std::unique_lock lock(publish_mutex);
        running_.store(false);
    }
    cv_.notify_all();
    if (core_process_.joinable())
    {
        core_process_.join();
    }
}

void AppController::publish(const std::string &topic, std::any data)
{
    {
        std::unique_lock lock(this->publish_mutex);
        this->publishers_[topic].push_back(std::move(data));
    }
    cv_.notify_one();
}

void AppController::registerSubscribe(const std::string &topic, std::shared_ptr<Subscriber> subscriber)
{
    std::unique_lock lock(this->subscribe_mutex);
    this->subscribers_[topic].push_back(subscriber);
}

void AppController::cancelSubscribe(const std::string &topic, std::shared_ptr<Subscriber> subscriber)
{
    std::unique_lock lock(this->subscribe_mutex);
    auto &subs = this->subscribers_[topic];
    subs.remove_if([&](const std::weak_ptr<Subscriber> &t)
                   {auto tp = t.lock(); return !tp || tp == subscriber; });
}

void AppController::signalHandler(int)
{
    running_.store(false);
    AppController::instance().publish("exit", false);
    AppController::instance().cv_.notify_all();
}

void AppController::coreProcess()
{
    while (true)
    {
        std::unordered_map<std::string, std::list<std::any>> local_events;

        {
            std::unique_lock lock(publish_mutex);
            cv_.wait(lock, [&]()
                     { return !running_.load() || !publishers_.empty(); });

            if (!running_.load() && publishers_.empty())
            {
                break;
            }

            local_events.swap(publishers_);
        }

        for (auto &[topic, events] : local_events)
        {
            std::vector<std::shared_ptr<Subscriber>> subs_copy;
            {
                std::shared_lock sub_lock(subscribe_mutex);
                auto it = subscribers_.find(topic);
                if (it != subscribers_.end())
                {
                    for (auto &weak_sub : it->second)
                    {
                        if (auto sub = weak_sub.lock())
                        {
                            subs_copy.push_back(sub);
                        }
                    }
                }
            }

            for (auto &event : events)
            {
                for (auto &sub : subs_copy)
                {
                    sub->handleEvent(topic, event);
                }
            }
        }
    }
}
