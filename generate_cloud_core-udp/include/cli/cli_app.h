#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <unordered_map>

#include "cmdline.h"

#include "subscriber.h"

class CliApp : public Subscriber, public std::enable_shared_from_this<CliApp>
{
public:
    enum class Method
    {
        Calibrating,
        Reconstructing,
        Merging
    };

    CliApp(const cmdline::parser &parser);
    ~CliApp();

    void subscribe(const std::string &topic) override;
    void unsubscribe(const std::string &topic) override;
    void handleEvent(const std::string &topic, const std::any &event) override;

    void run();

private:
    std::atomic<bool> running_{false};
    std::string json_config_path_;
    Method method_;

    std::unordered_map<std::string, std::function<void(const std::any &)>> handle_map_;
    std::unordered_map<Method, std::function<void(const std::string &)>> method_map_;
};
