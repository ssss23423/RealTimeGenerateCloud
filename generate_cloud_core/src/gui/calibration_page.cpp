#include "calibration_page.h"

#include <string>

#include "imgui.h"

#include "app_controller.h"

void CalibrationPage::draw()
{
    char images_num_buf[32] = {'\0'};
    static float images_num_progress = 0.0f;

    if (this->progress_bar_right_image_count_ == 0)
    {
        sprintf(images_num_buf, "NaN");
    }
    else
    {
        images_num_progress = this->processed_image_count_ * 1.0f / this->progress_bar_right_image_count_;
        sprintf(images_num_buf, "%d/%d", this->processed_image_count_, this->progress_bar_right_image_count_);
    }

    ImGui::ProgressBar(images_num_progress, ImVec2(0.f, 0.f), images_num_buf);

    if (ImGui::Button("Calibration"))
    {
        AppController::instance().publish("task_command", std::make_any<std::pair<std::string, std::string>>("calibration", "start"));
    }
}

void CalibrationPage::updateProcessedImageCount(int val)
{
    this->processed_image_count_ = val;
}

void CalibrationPage::updateProgressBarRightFrameCount(int val)
{
    this->progress_bar_right_image_count_ = val;
}