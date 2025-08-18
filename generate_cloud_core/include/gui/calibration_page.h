#pragma once

class CalibrationPage
{
public:
    CalibrationPage() = default;
    void draw();

    void updateProcessedImageCount(int val);
    void updateProgressBarRightFrameCount(int val);

private:
    int processed_image_count_;
    int progress_bar_right_image_count_;
};
