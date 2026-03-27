#include "params_panel.h"

#include "logger.h"
#include "system_params.h"
#include "disabled_block.h"

ParamsPanel::ParamsPanel()
{
    event_map_["params_init_finished"] = [this](const std::any &event)
    {
        if (auto e = std::any_cast<ParamsInitEvent>(&event))
        {
            this->paramsInit(*e);
        }
    };

    event_map_["params_update"] = [this](const std::any &event)
    {
        if (auto e = std::any_cast<ParamUpdateEvent>(&event))
        {
            if (e->origin == "task")
            {
                this->paramsUpdate(*e);
            }
        }
    };

    this->initParamsUpdateMap();
}

void ParamsPanel::render()
{
    ImGui::Begin("Params Panel");

    try
    {
        ImGuiDisabledBlock disabled_block(state_.disable);
        ImGui::SeparatorText("Paths");
        {
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Config File");
            ImGui::SameLine(120);
            if (ImGui::InputText("##Config File", state_.config_file_path, IM_ARRAYSIZE(state_.config_file_path), ImGuiInputTextFlags_EnterReturnsTrue))
            {
                AppController::instance().publish("params_reload", std::make_any<std::pair<std::string, std::string>>(state_.config_file_path, "gui"));
                AppController::instance().getLogger().log("Modified config file to " + std::string(state_.config_file_path));
            }

            ImGui::AlignTextToFramePadding();
            ImGui::Text("Poses Dir");
            ImGui::SameLine(120);
            if (ImGui::InputText("##Poses Dir", state_.poses_dir, IM_ARRAYSIZE(state_.poses_dir), ImGuiInputTextFlags_EnterReturnsTrue))
            {
                auto full_path = std::filesystem::weakly_canonical(std::filesystem::path(state_.base_dir) / std::filesystem::path(state_.poses_dir));
                AppController::instance().publish("params_update", std::make_any<ParamUpdateEvent>("poses_dir", std::make_any<std::string>(full_path.string()), "ui"));
                AppController::instance().getLogger().log("Modified poses dir to " + full_path.string());
            }
        }

        ImGui::SeparatorText("Calibration Params");
        {
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Image Dir");
            ImGui::SameLine(120);
            if (ImGui::InputText("##Calibration Image Dir", state_.calibration_images_dir, IM_ARRAYSIZE(state_.calibration_images_dir), ImGuiInputTextFlags_EnterReturnsTrue))
            {
                auto full_path = std::filesystem::weakly_canonical(std::filesystem::path(state_.base_dir) / std::filesystem::path(state_.calibration_images_dir));
                AppController::instance().publish("params_update", std::make_any<ParamUpdateEvent>("calibration_images_dir", std::make_any<std::string>(full_path.string()), "ui"));
                AppController::instance().getLogger().log("Modified calibration image dir to " + full_path.string());
            }

            ImGui::AlignTextToFramePadding();
            ImGui::Text("Caltab Descr");
            ImGui::SameLine(120);
            if (ImGui::InputText("##Caltab Description", state_.caltab_description, IM_ARRAYSIZE(state_.caltab_description), ImGuiInputTextFlags_EnterReturnsTrue))
            {
                auto full_path = std::filesystem::weakly_canonical(std::filesystem::path(state_.base_dir) / std::filesystem::path(state_.caltab_description));
                AppController::instance().publish("params_update", std::make_any<ParamUpdateEvent>("caltab_description", std::make_any<std::string>(full_path.string()), "ui"));
                AppController::instance().getLogger().log("Modified caltab description to " + full_path.string());
            }

            ImGui::AlignTextToFramePadding();
            ImGui::Text("Image Count");
            ImGui::SameLine(120);
            ImGui::SliderInt("##Calibration Image Count", &state_.calibration_image_count, 1, 500, "%d", ImGuiSliderFlags_AlwaysClamp);
            if (ImGui::IsItemDeactivatedAfterEdit())
            {
                AppController::instance().publish("params_update", std::make_any<ParamUpdateEvent>("calibration_image_count", std::make_any<int>(state_.calibration_image_count), "ui"));
                AppController::instance().getLogger().log("Modified calibration images num to " + std::to_string(state_.calibration_image_count));
            }

            ImGui::AlignTextToFramePadding();
            ImGui::Text("Min Threshold");
            ImGui::SameLine(120);
            ImGui::SliderInt("##Calibration Min Threshold", &state_.calibration_min_threshold, 0, 255, "%d", ImGuiSliderFlags_AlwaysClamp);
            if (ImGui::IsItemDeactivatedAfterEdit())
            {
                AppController::instance().publish("params_update", std::make_any<ParamUpdateEvent>("calibration_min_threshold", std::make_any<int>(state_.calibration_min_threshold), "ui"));
                AppController::instance().getLogger().log("Modified calibration min threshold to " + std::to_string(state_.calibration_min_threshold));
            }

            ImGui::AlignTextToFramePadding();
            ImGui::Text("Step Count");
            ImGui::SameLine(120);
            ImGui::SliderInt("##Calibration Step Count", &state_.calibration_step_count, 1, 6000, "%d", ImGuiSliderFlags_AlwaysClamp);
            if (ImGui::IsItemDeactivatedAfterEdit())
            {
                AppController::instance().publish("params_update", std::make_any<ParamUpdateEvent>("calibration_step_count", std::make_any<int>(state_.calibration_step_count), "ui"));
                AppController::instance().getLogger().log("Modified calibration step count to " + std::to_string(state_.calibration_step_count));
            }

            ImGui::AlignTextToFramePadding();
            ImGui::Text("Thickness");
            ImGui::SameLine(120);
            ImGui::SliderFloat("##Calibration Thickness", &state_.calibration_thickness, 0, 1, "%.2f", ImGuiSliderFlags_AlwaysClamp);
            if (ImGui::IsItemDeactivatedAfterEdit())
            {
                AppController::instance().publish("params_update", std::make_any<ParamUpdateEvent>("calibration_thickness", std::make_any<float>(state_.calibration_thickness), "ui"));
                AppController::instance().getLogger().log("Modified calibration thickness to " + std::to_string(state_.calibration_thickness));
            }

            ImGui::AlignTextToFramePadding();
            ImGui::Text("Max Error");
            ImGui::SameLine(120);
            ImGui::SliderFloat("##Calibration Max Light Plane Error", &state_.calibration_max_light_plane_error, 0, 1, "%.2f", ImGuiSliderFlags_AlwaysClamp);
            if (ImGui::IsItemDeactivatedAfterEdit())
            {
                AppController::instance().publish("params_update", std::make_any<ParamUpdateEvent>("calibration_max_light_plane_error", std::make_any<float>(state_.calibration_max_light_plane_error), "ui"));
                AppController::instance().getLogger().log("Modified max light plane error to " + std::to_string(state_.calibration_max_light_plane_error));
            }
        }

        ImGui::SeparatorText("Reconstruction Params");
        {
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Image Dir");
            ImGui::SameLine(120);
            if (ImGui::InputText("##Reconstruction Image Dir", state_.reconstruction_image_data, IM_ARRAYSIZE(state_.reconstruction_image_data), ImGuiInputTextFlags_EnterReturnsTrue))
            {
                auto full_path = std::filesystem::weakly_canonical(std::filesystem::path(state_.base_dir) / std::filesystem::path(state_.reconstruction_image_data));
                AppController::instance().publish("params_update", std::make_any<ParamUpdateEvent>("reconstruction_image_data", std::make_any<std::string>(full_path.string()), "ui"));
                AppController::instance().getLogger().log("Modified reconstruction image dir to " + full_path.string());
            }

            ImGui::AlignTextToFramePadding();
            ImGui::Text("Ouput Dir");
            ImGui::SameLine(120);
            if (ImGui::InputText("##Reconstruction Output Dir", state_.reconstruction_output_cloud_dir, IM_ARRAYSIZE(state_.reconstruction_output_cloud_dir), ImGuiInputTextFlags_EnterReturnsTrue))
            {
                auto full_path = std::filesystem::weakly_canonical(std::filesystem::path(state_.base_dir) / std::filesystem::path(state_.reconstruction_output_cloud_dir));
                AppController::instance().publish("params_update", std::make_any<ParamUpdateEvent>("reconstruction_output_cloud_dir", std::make_any<std::string>(full_path.string()), "ui"));
                AppController::instance().getLogger().log("Modified reconstruction output dir to " + full_path.string());
            }

            ImGui::AlignTextToFramePadding();
            ImGui::Text("Min Threshold");
            ImGui::SameLine(120);
            ImGui::SliderInt("##Reconstruction Min Threshold", &state_.reconstruction_min_threshold, 0, 255, "%d", ImGuiSliderFlags_AlwaysClamp);
            if (ImGui::IsItemDeactivatedAfterEdit())
            {
                AppController::instance().publish("params_update", std::make_any<ParamUpdateEvent>("reconstruction_min_threshold", std::make_any<int>(state_.reconstruction_min_threshold), "ui"));
                AppController::instance().getLogger().log("Modified reconstruction min threshold to " + std::to_string(state_.reconstruction_min_threshold));
            }

            ImGui::AlignTextToFramePadding();
            ImGui::Text("Roi Area");
            ImGui::SameLine(120);
            if (state_.select_entire_frame)
            {
#ifdef _WIN32
                ImGui::TextDisabled(std::string(std::to_string(state_.reconstruction_roi_row1) + ", " +
                                                std::to_string(state_.reconstruction_roi_col1) + ", " +
                                                std::to_string(state_.reconstruction_roi_row2) + ", " +
                                                std::to_string(state_.reconstruction_roi_col2))
                                        .c_str());
#else
                ImGui::TextDisabled("%d, %d, %d, %d", state_.reconstruction_roi_row1, state_.reconstruction_roi_col1, state_.reconstruction_roi_row2, state_.reconstruction_roi_col2);
#endif
            }
            else
            {
#ifdef _WIN32
                ImGui::Text(std::string(std::to_string(state_.reconstruction_roi_row1) + ", " +
                                        std::to_string(state_.reconstruction_roi_col1) + ", " +
                                        std::to_string(state_.reconstruction_roi_row2) + ", " +
                                        std::to_string(state_.reconstruction_roi_col2))
                                .c_str());
#else
                ImGui::Text("%d, %d, %d, %d", state_.reconstruction_roi_row1, state_.reconstruction_roi_col1, state_.reconstruction_roi_row2, state_.reconstruction_roi_col2);
#endif
            }
        }

        ImGui::SeparatorText("Mode Settings");
        {
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Save Cloud");
            ImGui::SameLine(120);
            if (ImGui::Checkbox("##Save Cloud", &state_.save_cloud_flag))
            {
                AppController::instance().publish("params_update", std::make_any<ParamUpdateEvent>("save_cloud_flag", std::make_any<bool>(state_.save_cloud_flag), "ui"));
            }
            //2026.1.18 add
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Save Lines");
            ImGui::SameLine(120);
            if (ImGui::Checkbox("##Save Lines", &state_.save_line_cloud_flag))
            {
                AppController::instance().publish("params_update", std::make_any<ParamUpdateEvent>("save_line_cloud_flag", std::make_any<bool>(state_.save_line_cloud_flag), "ui"));
            }

            ImGui::AlignTextToFramePadding();
            ImGui::Text("Line Stride");
            ImGui::SameLine(120);
            if (ImGui::InputInt("##Line Stride", &state_.line_cloud_stride))
            {
                if (state_.line_cloud_stride < 1) state_.line_cloud_stride = 1;
                AppController::instance().publish("params_update",
                    std::make_any<ParamUpdateEvent>("line_cloud_stride",
                        std::make_any<int>(state_.line_cloud_stride), "ui"));
            }

            ImGui::AlignTextToFramePadding();
            ImGui::Text("Line File Type");
            ImGui::SameLine(120);
            if (ImGui::InputText("##Line File Type", state_.line_cloud_file_type, sizeof(state_.line_cloud_file_type)))
            {
                AppController::instance().publish("params_update",
                    std::make_any<ParamUpdateEvent>("line_cloud_file_type",
                        std::make_any<std::string>(std::string(state_.line_cloud_file_type)), "ui"));
            }
            //2026.1.18 add finish

            {
                ImGuiDisabledBlock disabled_setting_roi(state_.select_entire_frame);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("Setting Roi");
                ImGui::SameLine(120);
                if (ImGui::Checkbox("##Setting Roi", &state_.set_roi_flag))
                {
                    AppController::instance().publish("params_update", std::make_any<ParamUpdateEvent>("set_roi_flag", std::make_any<bool>(state_.set_roi_flag), "ui"));
                }
            }

            ImGui::AlignTextToFramePadding();
            ImGui::Text("Entire Frame");
            ImGui::SameLine(120);
            if (ImGui::Checkbox("##Entire Frame", &state_.select_entire_frame))
            {
                AppController::instance().publish("params_update", std::make_any<ParamUpdateEvent>("select_entire_frame", std::make_any<bool>(state_.select_entire_frame), "ui"));
            }
        }

        //2026.3.1 add udp
        ImGui::SeparatorText("UDP Streaming");
        {
            // Enable
            ImGui::AlignTextToFramePadding();
            ImGui::Text("UDP Send");
            ImGui::SameLine(120);
            if (ImGui::Checkbox("##UDP Send", &state_.udp_send_enabled))
            {
                AppController::instance().publish("params_update",
                    std::make_any<ParamUpdateEvent>("udp_send_enabled",
                        std::make_any<bool>(state_.udp_send_enabled), "ui"));
            }

            // Host
            ImGui::AlignTextToFramePadding();
            ImGui::Text("UDP Host");
            ImGui::SameLine(120);
            if (ImGui::InputText("##UDP Host", state_.udp_host, sizeof(state_.udp_host)))
            {
                AppController::instance().publish("params_update",
                    std::make_any<ParamUpdateEvent>("udp_host",
                        std::make_any<std::string>(std::string(state_.udp_host)), "ui"));
            }

            // Port
            ImGui::AlignTextToFramePadding();
            ImGui::Text("UDP Port");
            ImGui::SameLine(120);
            if (ImGui::InputInt("##UDP Port", &state_.udp_port))
            {
                if (state_.udp_port < 1) state_.udp_port = 1;
                if (state_.udp_port > 65535) state_.udp_port = 65535;
                AppController::instance().publish("params_update",
                    std::make_any<ParamUpdateEvent>("udp_port",
                        std::make_any<int>(state_.udp_port), "ui"));
            }

            // MTU
            ImGui::AlignTextToFramePadding();
            ImGui::Text("UDP MTU");
            ImGui::SameLine(120);
            if (ImGui::InputInt("##UDP MTU", &state_.udp_mtu))
            {
                if (state_.udp_mtu < 256) state_.udp_mtu = 256;
                if (state_.udp_mtu > 65000) state_.udp_mtu = 65000;
                AppController::instance().publish("params_update",
                    std::make_any<ParamUpdateEvent>("udp_mtu",
                        std::make_any<int>(state_.udp_mtu), "ui"));
            }
        }
        //2026.3.1 add finish

    }
    catch (const std::exception &e)
    {
        std::string msg = "[Setting Params] " + std::string(e.what());
        AppController::instance().getLogger().log(msg, Logger::LogLevel::Error);
    }

    ImGui::End();
}

void ParamsPanel::handleEvent(const std::string &topic, const std::any &event)
{
    auto it = event_map_.find(topic);
    if (it != event_map_.end())
    {
        it->second(event);
    }
    else
    {
        AppController::instance().getLogger().log("No handler for topic: " + topic);
    }
}

void ParamsPanel::safe_copy(char *dest, const char *src, size_t N)
{
    if (!src)
    {
        AppController::instance().getLogger().log("[Ui] Error when copy", Logger::LogLevel::Error);
    }
    else
    {
        std::strncpy(dest, src, N - 1);
        dest[N - 1] = '\0';
    }
}

void ParamsPanel::initParamsUpdateMap()
{
    params_update_map_["disable"] = [this](const ParamUpdateEvent &e)
    {
        this->state_.disable = std::any_cast<bool>(e.value);
    };

    params_update_map_["config_file_path"] = [this](const ParamUpdateEvent &e)
    {
        ParamsPanel::safe_copy(this->state_.config_file_path, std::any_cast<const std::string &>(e.value).c_str(), sizeof(this->state_.config_file_path));
    };

    params_update_map_["poses_dir"] = [this](const ParamUpdateEvent &e)
    {
        ParamsPanel::safe_copy(this->state_.poses_dir, std::any_cast<const std::string &>(e.value).c_str(), sizeof(this->state_.poses_dir));
    };

    params_update_map_["calibration_images_dir"] = [this](const ParamUpdateEvent &e)
    {
        ParamsPanel::safe_copy(this->state_.calibration_images_dir, std::any_cast<const std::string &>(e.value).c_str(), sizeof(this->state_.calibration_images_dir));
    };

    params_update_map_["caltab_description"] = [this](const ParamUpdateEvent &e)
    {
        ParamsPanel::safe_copy(this->state_.caltab_description, std::any_cast<const std::string &>(e.value).c_str(), sizeof(this->state_.caltab_description));
    };

    params_update_map_["calibration_image_count"] = [this](const ParamUpdateEvent &e)
    {
        this->state_.calibration_image_count = std::any_cast<int>(e.value);
    };

    params_update_map_["calibration_min_threshold"] = [this](const ParamUpdateEvent &e)
    {
        this->state_.calibration_min_threshold = std::any_cast<int>(e.value);
    };

    params_update_map_["calibration_step_count"] = [this](const ParamUpdateEvent &e)
    {
        this->state_.calibration_step_count = std::any_cast<int>(e.value);
    };

    params_update_map_["calibration_thickness"] = [this](const ParamUpdateEvent &e)
    {
        this->state_.calibration_thickness = std::any_cast<float>(e.value);
    };

    params_update_map_["calibration_max_light_plane_error"] = [this](const ParamUpdateEvent &e)
    {
        this->state_.calibration_max_light_plane_error = std::any_cast<float>(e.value);
    };

    params_update_map_["reconstruction_image_data"] = [this](const ParamUpdateEvent &e)
    {
        ParamsPanel::safe_copy(this->state_.reconstruction_image_data, std::any_cast<const std::string &>(e.value).c_str(), sizeof(this->state_.reconstruction_image_data));
    };

    params_update_map_["reconstruction_output_cloud_dir"] = [this](const ParamUpdateEvent &e)
    {
        ParamsPanel::safe_copy(this->state_.reconstruction_output_cloud_dir, std::any_cast<const std::string &>(e.value).c_str(), sizeof(this->state_.reconstruction_output_cloud_dir));
    };

    params_update_map_["reconstruction_min_threshold"] = [this](const ParamUpdateEvent &e)
    {
        this->state_.reconstruction_min_threshold = std::any_cast<int>(e.value);
    };

    params_update_map_["reconstruction_roi_row1"] = [this](const ParamUpdateEvent &e)
    {
        this->state_.reconstruction_roi_row1 = std::any_cast<int>(e.value);
    };

    params_update_map_["reconstruction_roi_col1"] = [this](const ParamUpdateEvent &e)
    {
        this->state_.reconstruction_roi_col1 = std::any_cast<int>(e.value);
    };

    params_update_map_["reconstruction_roi_row2"] = [this](const ParamUpdateEvent &e)
    {
        this->state_.reconstruction_roi_row2 = std::any_cast<int>(e.value);
    };

    params_update_map_["reconstruction_roi_col2"] = [this](const ParamUpdateEvent &e)
    {
        this->state_.reconstruction_roi_col2 = std::any_cast<int>(e.value);
    };

    params_update_map_["save_cloud_flag"] = [this](const ParamUpdateEvent &e)
    {
        this->state_.save_cloud_flag = std::any_cast<bool>(e.value);
    };
    //2026.1.18 add
    params_update_map_["save_line_cloud_flag"] = [this](const ParamUpdateEvent &e)
    {
        this->state_.save_line_cloud_flag = std::any_cast<bool>(e.value);
    };
    params_update_map_["line_cloud_stride"] = [this](const ParamUpdateEvent &e)
    {
        this->state_.line_cloud_stride = std::any_cast<int>(e.value);
        if (this->state_.line_cloud_stride < 1) this->state_.line_cloud_stride = 1;
    };

    params_update_map_["line_cloud_file_type"] = [this](const ParamUpdateEvent &e)
    {
        std::string v = std::any_cast<const std::string &>(e.value);
#ifdef _WIN32
        strncpy_s(this->state_.line_cloud_file_type, sizeof(this->state_.line_cloud_file_type), v.c_str(), _TRUNCATE);
#else
        std::strncpy(this->state_.line_cloud_file_type, v.c_str(), sizeof(this->state_.line_cloud_file_type) - 1);
        this->state_.line_cloud_file_type[sizeof(this->state_.line_cloud_file_type) - 1] = '\0';
#endif
    };
    //2026.1.18 add finish

    //2026.3.1 add udp
    params_update_map_["udp_send_enabled"] = [this](const ParamUpdateEvent &e)
    {
        this->state_.udp_send_enabled = std::any_cast<bool>(e.value);
    };
    params_update_map_["udp_host"] = [this](const ParamUpdateEvent &e)
    {
        std::string v = std::any_cast<const std::string &>(e.value);
    #ifdef _WIN32
        strncpy_s(this->state_.udp_host, sizeof(this->state_.udp_host), v.c_str(), _TRUNCATE);
    #else
        std::strncpy(this->state_.udp_host, v.c_str(), sizeof(this->state_.udp_host) - 1);
        this->state_.udp_host[sizeof(this->state_.udp_host) - 1] = '\0';
    #endif
    };

    params_update_map_["udp_port"] = [this](const ParamUpdateEvent &e)
    {
        this->state_.udp_port = std::any_cast<int>(e.value);
    };
    params_update_map_["udp_mtu"] = [this](const ParamUpdateEvent &e)
    {
        this->state_.udp_mtu = std::any_cast<int>(e.value);
    };

    //2026.3.1 add finish


    params_update_map_["set_roi_flag"] = [this](const ParamUpdateEvent &e)
    {
        this->state_.set_roi_flag = std::any_cast<bool>(e.value);
    };

    params_update_map_["select_entire_frame"] = [this](const ParamUpdateEvent &e)
    {
        this->state_.select_entire_frame = std::any_cast<bool>(e.value);
    };
}

void ParamsPanel::paramsInit(const ParamsInitEvent &e)
{
    state_ = e.params;
}

void ParamsPanel::paramsUpdate(const ParamUpdateEvent &e)
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
