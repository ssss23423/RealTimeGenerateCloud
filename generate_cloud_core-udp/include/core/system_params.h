#pragma once

#include <atomic>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

#include <HalconCpp.h>

#include "nlohmann/json.hpp"

using namespace HalconCpp;
using json = nlohmann::json;

struct hvProgramParams
{
    // Data path
    HTuple hv_reconstruction_image_data_path;
    HTuple hv_calibration_images_dir;
    HTuple hv_poses_dir;
    HTuple hv_laser1_path;
    HTuple hv_laser2_path;
    HTuple hv_movement1_path;
    HTuple hv_movement2_path;
    HTuple hv_caltab_description_path;
    HTuple hv_reconstruction_output_dir;
    std::vector<HTuple> hv_output_cloud_paths;

    // Calibration parameters
    HTuple hv_calibration_image_count;
    HTuple hv_calibration_min_threshold;
    HTuple hv_calibration_step_count;
    HTuple hv_calibration_thickness;
    HTuple hv_calibration_max_light_plane_error;

    // Reconstruction parameters
    HTuple hv_reconstruction_min_threshold;
    HTuple hv_reconstruction_roi_row1;
    HTuple hv_reconstruction_roi_col1;
    HTuple hv_reconstruction_roi_row2;
    HTuple hv_reconstruction_roi_col2;
};

struct hvSystemPoses
{
    HTuple hv_start_params;
    HTuple hv_camera_params;
    HTuple hv_camera_pose;
    HTuple hv_light_plane_poses;
    HTuple hv_movement_poses;
};

struct RealTimeParams
{
    std::string host;
    std::string user;
    int port;
    std::string password;
    std::string remote_data_dir;
    std::string local_temp_dir;
    int scan_interval_seconds;
    std::string local_simulation_dir;
};

enum class Path
{
    PosesDir,

    CaltabDescription,
    CalibrationImageDir,

    ReconstructionImageDir,
    ReconstructionOutputDir,

    CameraParameters,
    CameraPose,
    LightPlanePose,
    MovementPose,

    All
};

enum class Mode
{
    Ui,
    Term
};

class SystemParams
{
public:
    static void init(const std::string &config_path, const Mode &mode);
    static SystemParams &instance();
    void reload(const std::string &new_config_path, const Mode &mode);
    Mode getMode();
    std::filesystem::path getConfigBaseDir();
    void validatePaths(Path path) const;

    hvProgramParams hv_program_params_;
    hvSystemPoses hv_system_poses_;

    RealTimeParams rt_params_;

    // Flag parameters
    mutable bool calibration_flag_ = true;
    bool save_cloud_flag_;
    bool set_roi_flag_;
    bool select_entire_frame_;
    
    //2026.1.18 add
    bool save_line_cloud_flag_ = false;
    int line_cloud_stride_ = 1;
    std::string line_cloud_file_type_ = "ply_binary";
    //2026.1.18 add finish

    //2026.3.1 add udp
    bool udp_send_enabled_ = true;
    std::string udp_host_ = "127.0.0.1";
    int udp_port_ = 9000;
    int udp_mtu_ = 1400;
    //2026.3.1 add finish

    bool set_threshold_flag_ = false;



    std::string config_path_;
    int img_width_;
    int img_height_;
    int img_size_;
    int fps_;

    int total_valid_files_ = 0;
    std::vector<std::string> valid_files_;
    std::vector<int> frame_count_;

private:
    explicit SystemParams(const std::string &config_path, const Mode &mode);
    SystemParams(const SystemParams &) = delete;
    SystemParams &operator=(const SystemParams &) = delete;

    void internalInitialize();
    void loadConfigFile();
    void loadPosesFiles();
    void initFinished();

    json config_;
    std::filesystem::path base_dir_;
    std::mutex mtx_;
    Mode mode_;
    static inline std::unique_ptr<SystemParams> instance_ = nullptr;
    static inline std::once_flag init_flag_;
};