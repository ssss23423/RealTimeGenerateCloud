#include "task_panel.h"

#include "imgui.h"

TaskPanel::TaskPanel()
{
    reconstruction_page_ = std::make_unique<ReconstructionPage>();
    calibration_page_ = std::make_unique<CalibrationPage>();

    this->initHandleMap();
}

// void TaskPanel::render()
// {
//     ImGui::Begin("Task Panel");

//     ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
//     if (ImGui::BeginTabBar("Task Panel Tab Bar", tab_bar_flags))
//     {
//         if (ImGui::BeginTabItem("Reconstruction"))
//         {
//             reconstruction_page_->draw();
//             ImGui::EndTabItem();
//         }

//         if (ImGui::BeginTabItem("Calibration"))
//         {
//             calibration_page_->draw();
//             ImGui::EndTabItem();
//         }

//         ImGui::EndTabBar();
//     }

//     ImGui::End();
// }

//rewrite render()
void TaskPanel::render()
{
    ImGui::Begin("Task Panel");
    ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
    if (ImGui::BeginTabBar("Task Panel Tab Bar", tab_bar_flags))
    {
        if (ImGui::BeginTabItem("Reconstruction"))
        {
            ImGui::SeparatorText("Batch Reconstruction");
            reconstruction_page_->draw();
            
            ImGui::Dummy(ImVec2(0.0f, 20.0f)); // Add some space
            ImGui::SeparatorText("Real-Time Reconstruction");
 
            if (ImGui::Button("Start Real-Time Scan"))
            {
                AppController::instance().publish("task_command", std::make_any<std::pair<std::string, std::string>>("real_time_reconstruction", "start"));
            }
            ImGui::SameLine();
            if (ImGui::Button("Stop Real-Time Scan"))
            {
                AppController::instance().publish("task_command", std::make_any<std::pair<std::string, std::string>>("real_time_reconstruction", "stop"));
            }
 
            ImGui::EndTabItem();
        }
 
        if (ImGui::BeginTabItem("Calibration"))
        {
            calibration_page_->draw();
            ImGui::EndTabItem();
        }
 
        ImGui::EndTabBar();
    }
 
    ImGui::End();
}

void TaskPanel::handleEvent(const std::string &topic, const std::any &event)
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

void TaskPanel::initHandleMap()
{
    handle_map_["params_update"] = [this](const std::any &event)
    {
        if (auto e = std::any_cast<ParamUpdateEvent>(&event))
        {
            if (e->origin == "task")
            {
                this->paramsUpdate(*e);
            }
        }
    };
}

void TaskPanel::initParamsUpdateMap()
{
    this->params_update_map_["calibration_processed_images_num_"] = [this](const ParamUpdateEvent &event)
    {
        int val = std::any_cast<int>(event.value);
        this->calibration_page_->updateProcessedImageCount(val);
    };

    this->params_update_map_["calibration_progress_bar_right_images_num"] = [this](const ParamUpdateEvent &event)
    {
        int val = std::any_cast<int>(event.value);
        this->calibration_page_->updateProgressBarRightFrameCount(val);
    };

    this->params_update_map_["reconstruction_processed_file_num"] = [this](const ParamUpdateEvent &event)
    {
        int val = std::any_cast<int>(event.value);
        this->reconstruction_page_->updateProcessedFileCount(val);
    };

    this->params_update_map_["reconstruction_processed_frame_num"] = [this](const ParamUpdateEvent &event)
    {
        int val = std::any_cast<int>(event.value);
        this->reconstruction_page_->updateProcessedFrameCount(val);
    };

    this->params_update_map_["reconstruction_progress_bar_right_valid_files_num"] = [this](const ParamUpdateEvent &event)
    {
        int val = std::any_cast<int>(event.value);
        this->reconstruction_page_->updateProgressBarRightValidFileCount(val);
    };

    this->params_update_map_["reconstruction_progress_bar_right_frame_num"] = [this](const ParamUpdateEvent &event)
    {
        int val = std::any_cast<int>(event.value);
        this->reconstruction_page_->updateProgressBarRightFrameCount(val);
    };
}

void TaskPanel::paramsUpdate(const ParamUpdateEvent &e)
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