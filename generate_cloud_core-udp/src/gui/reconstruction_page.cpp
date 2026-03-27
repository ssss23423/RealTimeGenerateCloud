#include "reconstruction_page.h"

#include <string>
#include "imgui.h"

#include "app_controller.h"

void ReconstructionPage::draw()
{
    char file_num_buf[32] = {'\0'};
    char frame_num_buf[32] = {'\0'};
    static float file_num_progress = 0.0f;
    static float frame_num_progress = 0.0f;

    if (this->progress_bar_right_valid_file_count_ == 0)
    {
        sprintf(file_num_buf, "NaN");
    }
    else
    {
        file_num_progress = this->processed_file_count_ / this->progress_bar_right_valid_file_count_;
        sprintf(file_num_buf, "%d/%d", this->processed_file_count_, this->progress_bar_right_valid_file_count_);
    }

    if (this->progress_bar_right_frame_count_ == 0)
    {
        sprintf(frame_num_buf, "NaN");
    }
    else
    {
        frame_num_progress = this->processed_frame_count_ / this->progress_bar_right_frame_count_;
        sprintf(frame_num_buf, "%d/%d", this->processed_frame_count_, this->progress_bar_right_frame_count_);
    }

    ImGui::ProgressBar(file_num_progress, ImVec2(0.f, 0.f), file_num_buf);
    ImGui::ProgressBar(frame_num_progress, ImVec2(0.f, 0.f), frame_num_buf);

    if (ImGui::Button("Reconstruction"))
    {
        AppController::instance().publish("task_command", std::make_any<std::pair<std::string, std::string>>("reconstruction", "start"));
    }
    ImGui::SameLine();
    if (ImGui::Button("Stop"))
    {
        AppController::instance().publish("task_command", std::make_any<std::pair<std::string, std::string>>("reconstruction", "stop"));
    }
}

void ReconstructionPage::updateProcessedFileCount(int val)
{
    this->processed_file_count_ = val;
}

void ReconstructionPage::updateProcessedFrameCount(int val)
{
    this->processed_frame_count_ = val;
}

void ReconstructionPage::updateProgressBarRightValidFileCount(int val)
{
    this->progress_bar_right_valid_file_count_ = val;
}

void ReconstructionPage::updateProgressBarRightFrameCount(int val)
{
    this->progress_bar_right_frame_count_ = val;
}
