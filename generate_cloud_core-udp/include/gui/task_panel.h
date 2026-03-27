#pragma once

#include <memory>
#include <unordered_map>

#include "app_controller.h"
#include "reconstruction_page.h"
#include "calibration_page.h"

class TaskPanel : public SubscriberAdapter
{
public:
    TaskPanel();

    void render();
    void handleEvent(const std::string &topic, const std::any &event) override;

private:
    std::unique_ptr<ReconstructionPage> reconstruction_page_ = nullptr;
    std::unique_ptr<CalibrationPage> calibration_page_ = nullptr;

    std::unordered_map<std::string, std::function<void(const std::any &)>> handle_map_;
    std::unordered_map<std::string, std::function<void(const ParamUpdateEvent &)>> params_update_map_;

    void initHandleMap();
    void initParamsUpdateMap();
    void paramsUpdate(const ParamUpdateEvent &e);
};
