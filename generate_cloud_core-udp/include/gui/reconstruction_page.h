#pragma once

class ReconstructionPage
{
public:
    ReconstructionPage() = default;
    void draw();

    void updateProcessedFileCount(int val);
    void updateProcessedFrameCount(int val);
    void updateProgressBarRightValidFileCount(int val);
    void updateProgressBarRightFrameCount(int val);

private:
    int processed_file_count_ = 0;
    int processed_frame_count_ = 0;
    int progress_bar_right_valid_file_count_ = 0;
    int progress_bar_right_frame_count_ = 0;
};