#pragma once

#include <any>
#include <string>

struct ParamsPanelState
{
    bool disable = true;

    char base_dir[256] = {'\0'};
    char config_file_path[256] = {'\0'};
    char poses_dir[256] = {'\0'};

    char calibration_images_dir[256] = {'\0'};
    char caltab_description[256] = {'\0'};
    int calibration_image_count = 0;
    int calibration_min_threshold = 0;
    int calibration_step_count = 0;
    float calibration_thickness = 0.0f;
    float calibration_max_light_plane_error = 0.0f;

    char reconstruction_image_data[256] = {'\0'};
    char reconstruction_output_cloud_dir[256] = {'\0'};
    int reconstruction_min_threshold = 0;
    int reconstruction_roi_row1 = 0;
    int reconstruction_roi_col1 = 0;
    int reconstruction_roi_row2 = 0;
    int reconstruction_roi_col2 = 0;

    bool save_cloud_flag = false;
    bool set_roi_flag = false;
    bool select_entire_frame = true;

    int calibration_processed_images_num = 0;
    int calibration_progress_bar_right_images_num = 0;

    int reconstruction_processed_file_num = 0;
    int reconstruction_processed_frame_num = 0;
    int reconstruction_progress_bar_right_valid_files_num = 0;
    int reconstruction_progress_bar_right_frame_num = 0;
};

struct ParamsInitEvent
{
    ParamsPanelState params;
};

struct ParamUpdateEvent
{
    std::string name;
    std::any value;
    std::string origin;
};
