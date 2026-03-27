#pragma once

#include <opencv2/opencv.hpp>

#include "task.h"
#include "system_params.h"

class ReconstructionTask : public Task
{
public:
    ReconstructionTask(std::string id);

    //new method for single file process
    void processSingleFile(const std::string& bil_file_path);
    void setExtraRunning(std::atomic<bool>* ext_running){
        external_running_ = ext_running;
    }

private:
    void run() override;

    void findDatafiles();
    void reconstruction();
    void extractLaser(const HObject &ho_image, HObject *ho_laser_image, const HTuple &hv_min_threshold);
    void resizeImage(const HObject &ho_image_part, const HTuple &hv_width,
                     const HTuple &hv_height, HObject *ho_resized_image,
                     const HTuple &hv_roi_row1, const HTuple &hv_roi_column1);
    std::string getTime();

    void preprocess();
    void setRoi(const cv::Mat &src_mat);
    void setThreshold(const cv::Mat &src_mat);
    void matToHObject(const cv::Mat &src_mat, HObject *dst_hobj);

    //new core method
    void performReconstruction();

    bool drawing_box_ = false;
    cv::Rect box_{0, 0, 0, 0};

    int temp_threshold_ = 0;
    cv::Mat src_mat_{};
    cv::Mat dst_mat_{};

    //new atrri
    std::atomic<bool>* external_running_{nullptr};
};
