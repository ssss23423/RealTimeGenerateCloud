#pragma once

#include "subscriber.h"

class SubscriberAdapter : public Subscriber
{
public:
    void subscribe(const std::string &topic) override {}
    void unsubscribe(const std::string &topic) override {}
    void handleEvent(const std::string &topic, const std::any &event) override {}
};
