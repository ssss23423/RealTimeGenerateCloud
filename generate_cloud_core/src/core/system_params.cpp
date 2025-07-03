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

    // Load the program parameters
    hv_program_params_.hv_data_root = config_["params"]["dataRoot"].get<std::string>().c_str();
    hv_program_params_.hv_caltab_description_path = config_["params"]["caltabDescription"].get<std::string>().c_str();
    hv_program_params_.hv_output_cloud_root = config_["params"]["outputCloudRoot"].get<std::string>().c_str();

    hv_program_params_.hv_calibration_images_num = config_["params"]["calibrationImagesNumber"].get<int>();
    hv_program_params_.hv_calibration_min_threshold = config_["params"]["calibrationMinThreshold"].get<int>();
    hv_program_params_.hv_calibration_thickness = config_["params"]["calibrationThickness"].get<float>();
    hv_program_params_.hv_calibration_steps_num = config_["params"]["calibrationStepsNumber"].get<int>();
    hv_program_params_.hv_calibration_max_light_plane_error = config_["params"]["calibrationMaxLightPlaneError"].get<float>();

    hv_program_params_.hv_reconstruction_min_threshold = config_["params"]["reconstructionMinThreshold"].get<int>();
    hv_program_params_.hv_reconstruction_roi_row1 = config_["params"]["reconstructionRoiRow1"].get<int>();
    hv_program_params_.hv_reconstruction_roi_col1 = config_["params"]["reconstructionRoiCol1"].get<int>();
    hv_program_params_.hv_reconstruction_roi_row2 = config_["params"]["reconstructionRoiRow2"].get<int>();
    hv_program_params_.hv_reconstruction_roi_col2 = config_["params"]["reconstructionRoiCol2"].get<int>();

    img_width_ = config_["params"]["imgWidth"].get<int>();
    img_height_ = config_["params"]["imgHeight"].get<int>();
    img_size_ = img_width_ * img_height_;

    this->save_cloud_flag_ = config_["mode"]["saveCloudFlag"].get<bool>();
    this->set_roi_flag_ = config_["mode"]["setRoiFlag"].get<bool>();
    this->select_entire_frame_ = config_["mode"]["selectEntireFrame"].get<bool>();

    const std::filesystem::path data_root(hv_program_params_.hv_data_root[0].S().Text());
    hv_program_params_.hv_calibration_images_path = (data_root / "caltab" / "caltab").string().c_str();
    hv_program_params_.hv_laser1_path = (data_root / "caltab" / "laser01").string().c_str();
    hv_program_params_.hv_laser2_path = (data_root / "caltab" / "laser02").string().c_str();
    hv_program_params_.hv_movement1_path = (data_root / "caltab" / "caltab_at_position_1").string().c_str();
    hv_program_params_.hv_movement2_path = (data_root / "caltab" / "caltab_at_position_2").string().c_str();
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
        CheckPath(hv_program_params_.hv_data_root, "Data root");
    }
    break;

    case Path::CaltabDescription:
    {
        CheckPath(hv_program_params_.hv_caltab_description_path, "Caltab description");
    }
    break;

    case Path::OutputModelRoot:
    {
        CheckPath(hv_program_params_.hv_output_cloud_root, "Output model root");
    }
    break;

    case Path::CameraParameters:
    {
        CheckPath(hv_program_params_.hv_data_root + "/saved_poses/camera_parameters.dat", "CameraParameters");
    }
    break;

    case Path::CameraPose:
    {
        CheckPath(hv_program_params_.hv_data_root + "/saved_poses/camera_pose.dat", "CameraPose");
    }
    break;

    case Path::LightPlanePose:
    {
        CheckPath(hv_program_params_.hv_data_root + "/saved_poses/light_plane_pose.dat", "LightPlanePose");
    }
    break;

    case Path::MovementPose:
    {
        CheckPath(hv_program_params_.hv_data_root + "/saved_poses/movement_pose.dat", "MovementPose");
    }
    break;

    case Path::All:
    default:
    {
        CheckPath(hv_program_params_.hv_data_root, "Data root");
        CheckPath(hv_program_params_.hv_caltab_description_path, "Caltab description");
        CheckPath(hv_program_params_.hv_output_cloud_root, "Output model root");
        CheckPath(hv_program_params_.hv_data_root + "/saved_poses/camera_parameters.dat", "CameraParameters");
        CheckPath(hv_program_params_.hv_data_root + "/saved_poses/camera_pose.dat", "CameraPose");
        CheckPath(hv_program_params_.hv_data_root + "/saved_poses/light_plane_pose.dat", "LightPlanePose");
        CheckPath(hv_program_params_.hv_data_root + "/saved_poses/movement_pose.dat", "MovementPose");
    }
    break;
    }
}

void SystemParams::loadPosesFiles()
{
    ReadCamPar(hv_program_params_.hv_data_root + "/saved_poses/camera_parameters.dat", &hv_system_poses_.hv_camera_params);
    ReadPose(hv_program_params_.hv_data_root + "/saved_poses/camera_pose.dat", &hv_system_poses_.hv_camera_pose);
    ReadPose(hv_program_params_.hv_data_root + "/saved_poses/light_plane_pose.dat", &hv_system_poses_.hv_light_plane_poses);
    ReadPose(hv_program_params_.hv_data_root + "/saved_poses/movement_pose.dat", &hv_system_poses_.hv_movement_poses);
    calibration_flag_ = false;
}