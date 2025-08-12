#pragma once
#include <any>
#include <string>

class Subscriber
{
public:
    virtual ~Subscriber() = default;
    virtual void subscribe(const std::string &topic) = 0;
    virtual void unsubscribe(const std::string &topic) = 0;
    virtual void handleEvent(const std::string &topic, const std::any &event) = 0;
};
