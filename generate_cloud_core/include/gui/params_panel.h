#pragma once

#include "imgui.h"
#include "app_controller.h"

class ParamsPanel : public SubscriberAdapter
{
public:
    ParamsPanel();

    void render();
    void handleEvent(const std::string &topic, const std::any &event) override;

private:
    std::unordered_map<std::string, std::function<void(const std::any &)>> event_map_;
    std::unordered_map<std::string, std::function<void(const ParamUpdateEvent &)>> params_update_map_;
    ParamsPanelState state_;

    static void safe_copy(char *dest, const char *src, size_t N);

    void initParamsUpdateMap();
    void paramsInit(const ParamsInitEvent &e);
    void paramsUpdate(const ParamUpdateEvent &e);
};