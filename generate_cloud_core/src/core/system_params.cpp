#include "system_params.h"

#include "logger.h"

SystemParams::SystemParams(const std::string &config_path, const RunMode &mode)
{
    run_mode_ = mode;
    reload(config_path);
}

void SystemParams::init(const std::string &config_path, const RunMode &mode)
{
    std::call_once(init_flag_, [&]()
                   { instance_ = std::unique_ptr<SystemParams>(new SystemParams(config_path, mode)); });
}

SystemParams &SystemParams::instance()
{
    if (!instance_)
    {
        throw std::runtime_error("SystemParams not initialized. Call init() first.");
    }
    return *instance_;
}

void SystemParams::reload(const std::string &new_config_path)
{
    std::lock_guard<std::mutex> lock(mtx_);
    config_path_ = new_config_path;
    internalInitialize();
}

RunMode SystemParams::getRunningMode()
{
    return run_mode_;
}

void SystemParams::internalInitialize()
{
    try
    {
        loadConfigFile();
        validatePaths(Path::All);
        loadPosesFiles();
        Logger::instance().log("[SystemParams] Load successfully!");
    }
    catch (const std::exception &e)
    {
        std::string msg = "[SystemParams] Reload failed: " + std::string(e.what());
        Logger::instance().log(msg, LogLevel::Error);
    }
    catch (const HException &e)
    {
        std::string msg = "[SystemParams] Reload failed: " + std::string(e.ErrorMessage());
        Logger::instance().log(msg, LogLevel::Error);
    }
}

void SystemParams::loadConfigFile()
{
    // Check if the config file exists
    if (!std::filesystem::exists(config_path_))
    {
        throw std::runtime_error("Config file missing: " + config_path_);
    }

    std::ifstream f(config_path_);
    config_ = json::parse(f);

    // Global Parameters
    this->img_width_ = config_["global"]["imageWidth"].get<int>();
    this->img_height_ = config_["global"]["imageHeight"].get<int>();
    this->img_size_ = this->img_width_ * this->img_height_;
    this->fps_ = config_["global"]["fps"].get<int>();

    // Load the program parameters
    this->hv_program_params_.hv_caltab_description_path = config_["calibration"]["caltabDescription"].get<std::string>().c_str();
    this->hv_program_params_.hv_calibration_images_dir = config_["calibration"]["caltabImagesDir"].get<std::string>().c_str();
    this->hv_program_params_.hv_calibration_image_count = config_["calibration"]["imageCount"].get<int>();
    this->hv_program_params_.hv_calibration_min_threshold = config_["calibration"]["minThreshold"].get<int>();
    this->hv_program_params_.hv_calibration_step_count = config_["calibration"]["stepCount"].get<int>();
    this->hv_program_params_.hv_calibration_thickness = config_["calibration"]["thickness"].get<float>();
    this->hv_program_params_.hv_calibration_max_light_plane_error = config_["calibration"]["maxLightPlaneError"].get<float>();

    this->hv_program_params_.hv_reconstruction_image_data_path = config_["reconstruction"]["imageData"].get<std::string>().c_str();
    this->hv_program_params_.hv_reconstruction_poses = config_["reconstruction"]["poses"].get<std::string>().c_str();
    this->hv_program_params_.hv_reconstruction_output_clouds_dir = config_["reconstruction"]["outputCloudsDir"].get<std::string>().c_str();
    this->hv_program_params_.hv_reconstruction_min_threshold = config_["reconstruction"]["minThreshold"].get<int>();
    this->hv_program_params_.hv_reconstruction_roi_row1 = config_["reconstruction"]["roiRow1"].get<int>();
    this->hv_program_params_.hv_reconstruction_roi_col1 = config_["reconstruction"]["roiCol1"].get<int>();
    this->hv_program_params_.hv_reconstruction_roi_row2 = config_["reconstruction"]["roiRow2"].get<int>();
    this->hv_program_params_.hv_reconstruction_roi_col2 = config_["reconstruction"]["roiCol2"].get<int>();

    // Mode parameters
    this->save_cloud_flag_ = config_["mode"]["saveCloudFlag"].get<bool>();
    this->set_roi_flag_ = config_["mode"]["setRoiFlag"].get<bool>();
    this->select_entire_frame_ = config_["mode"]["selectEntireFrame"].get<bool>();

    this->hv_program_params_.hv_laser1_path = this->hv_program_params_.hv_calibration_images_dir + "/laser01";
    this->hv_program_params_.hv_laser2_path = this->hv_program_params_.hv_calibration_images_dir + "/laser02";
    this->hv_program_params_.hv_movement1_path = this->hv_program_params_.hv_calibration_images_dir + "/caltab_at_position_1";
    this->hv_program_params_.hv_movement2_path = this->hv_program_params_.hv_calibration_images_dir + "/caltab_at_position_2";
}

void SystemParams::validatePaths(Path path) const
{
    auto CheckPath = [this](const HTuple &path, const std::string &name)
    {
        if (!std::filesystem::exists(path[0].S().Text()))
        {
            this->calibration_flag_ = true;
            throw std::runtime_error(name + " path invalid: " + path[0].S().Text());
        }
    };

    switch (path)
    {
    case Path::DataRoot:
    {
        CheckPath(hv_program_params_.hv_reconstruction_image_data_path, "Reconstruction data dir");
    }
    break;

    case Path::CaltabDescription:
    {
        CheckPath(hv_program_params_.hv_caltab_description_path, "Caltab description");
    }
    break;

    case Path::OutputModelRoot:
    {
        CheckPath(hv_program_params_.hv_reconstruction_output_clouds_dir, "Output model root");
    }
    break;

    case Path::CameraParameters:
    {
        CheckPath(hv_program_params_.hv_reconstruction_poses + "/camera_parameters.dat", "CameraParameters");
    }
    break;

    case Path::CameraPose:
    {
        CheckPath(hv_program_params_.hv_reconstruction_poses + "/camera_pose.dat", "CameraPose");
    }
    break;

    case Path::LightPlanePose:
    {
        CheckPath(hv_program_params_.hv_reconstruction_poses + "/light_plane_pose.dat", "LightPlanePose");
    }
    break;

    case Path::MovementPose:
    {
        CheckPath(hv_program_params_.hv_reconstruction_poses + "/movement_pose.dat", "MovementPose");
    }
    break;

    case Path::All:
    default:
    {
        CheckPath(hv_program_params_.hv_reconstruction_image_data_path, "Reconstruction data dir");
        CheckPath(hv_program_params_.hv_caltab_description_path, "Caltab description");
        CheckPath(hv_program_params_.hv_reconstruction_output_clouds_dir, "Output model root");
        CheckPath(hv_program_params_.hv_reconstruction_poses + "/camera_parameters.dat", "CameraParameters");
        CheckPath(hv_program_params_.hv_reconstruction_poses + "/camera_pose.dat", "CameraPose");
        CheckPath(hv_program_params_.hv_reconstruction_poses + "/light_plane_pose.dat", "LightPlanePose");
        CheckPath(hv_program_params_.hv_reconstruction_poses + "/movement_pose.dat", "MovementPose");
    }
    break;
    }
}

void SystemParams::loadPosesFiles()
{
    ReadCamPar(hv_program_params_.hv_reconstruction_poses + "/camera_parameters.dat", &hv_system_poses_.hv_camera_params);
    ReadPose(hv_program_params_.hv_reconstruction_poses + "/camera_pose.dat", &hv_system_poses_.hv_camera_pose);
    ReadPose(hv_program_params_.hv_reconstruction_poses + "/light_plane_pose.dat", &hv_system_poses_.hv_light_plane_poses);
    ReadPose(hv_program_params_.hv_reconstruction_poses + "/movement_pose.dat", &hv_system_poses_.hv_movement_poses);
    calibration_flag_ = false;
}