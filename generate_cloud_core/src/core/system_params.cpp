#include "system_params.h"

#include "app_controller.h"

SystemParams::SystemParams(const std::string &config_path, const Mode &mode)
{
    mode_ = mode;
    reload(config_path, mode_);
}

void SystemParams::init(const std::string &config_path, const Mode &mode)
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

void SystemParams::reload(const std::string &new_config_path, const Mode &mode)
{
    std::lock_guard<std::mutex> lock(mtx_);
    config_path_ = new_config_path;
    internalInitialize();
}

Mode SystemParams::getMode()
{
    return mode_;
}

std::filesystem::path SystemParams::getConfigBaseDir()
{
    return base_dir_;
}

void SystemParams::internalInitialize()
{
    try
    {
        this->loadConfigFile();
        this->validatePaths(Path::All);
        this->loadPosesFiles();
        AppController::instance().getLogger().log("[SystemParams] Load successfully!");
        this->initFinished();
    }
    catch (const std::exception &e)
    {
        std::string msg = "[SystemParams] Reload failed: " + std::string(e.what());
        AppController::instance().getLogger().log(msg, Logger::LogLevel::Error);
        if (this->mode_ == Mode::Term)
        {
            throw std::runtime_error(msg);
        }
    }
    catch (const HException &e)
    {
        std::string msg = "[SystemParams] Reload failed: " + std::string(e.ErrorMessage());
        AppController::instance().getLogger().log(msg, Logger::LogLevel::Error);
        if (this->mode_ == Mode::Term)
        {
            throw std::runtime_error(msg);
        }
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
    base_dir_ = std::filesystem::path(config_path_).parent_path();

    // Global Parameters
    this->img_width_ = config_["global"]["imageWidth"].get<int>();
    this->img_height_ = config_["global"]["imageHeight"].get<int>();
    this->img_size_ = this->img_width_ * this->img_height_;
    this->fps_ = config_["global"]["fps"].get<int>();
    this->hv_program_params_.hv_poses_dir = std::filesystem::weakly_canonical(base_dir_ / config_["global"]["poses"].get<std::string>()).c_str();

    // Load the program parameters
    this->hv_program_params_.hv_caltab_description_path = std::filesystem::weakly_canonical(base_dir_ / config_["global"]["caltabDescription"].get<std::string>()).c_str();
    this->hv_program_params_.hv_calibration_images_dir = std::filesystem::weakly_canonical(base_dir_ / config_["calibration"]["caltabImagesDir"].get<std::string>()).c_str();
    this->hv_program_params_.hv_calibration_image_count = config_["calibration"]["imageCount"].get<int>();
    this->hv_program_params_.hv_calibration_min_threshold = config_["calibration"]["minThreshold"].get<int>();
    this->hv_program_params_.hv_calibration_step_count = config_["calibration"]["stepCount"].get<int>();
    this->hv_program_params_.hv_calibration_thickness = config_["calibration"]["thickness"].get<float>();
    this->hv_program_params_.hv_calibration_max_light_plane_error = config_["calibration"]["maxLightPlaneError"].get<float>();

    this->hv_program_params_.hv_reconstruction_image_data_path = std::filesystem::weakly_canonical(base_dir_ / config_["reconstruction"]["imageData"].get<std::string>()).c_str();
    this->hv_program_params_.hv_reconstruction_output_dir = std::filesystem::weakly_canonical(base_dir_ / config_["reconstruction"]["outputCloudsDir"].get<std::string>()).c_str();
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

void SystemParams::loadPosesFiles()
{
    ReadCamPar(hv_program_params_.hv_poses_dir + "/camera_parameters.dat", &hv_system_poses_.hv_camera_params);
    ReadPose(hv_program_params_.hv_poses_dir + "/camera_pose.dat", &hv_system_poses_.hv_camera_pose);
    ReadPose(hv_program_params_.hv_poses_dir + "/light_plane_pose.dat", &hv_system_poses_.hv_light_plane_poses);
    ReadPose(hv_program_params_.hv_poses_dir + "/movement_pose.dat", &hv_system_poses_.hv_movement_poses);
    calibration_flag_ = false;
}

void SystemParams::validatePaths(Path path) const
{
    auto CheckPath = [this](const HTuple &path, const std::string &name)
    {
        if (!std::filesystem::exists(path[0].S().Text()) || path[0].S().IsEmpty())
        {
            this->calibration_flag_ = true;
            throw std::runtime_error(name + " path invalid: " + path[0].S().Text());
        }
    };

    switch (path)
    {
    case Path::PosesDir:
    {
        CheckPath(hv_program_params_.hv_poses_dir, "Poses dir");
    }
    break;

    case Path::CaltabDescription:
    {
        CheckPath(hv_program_params_.hv_caltab_description_path, "Caltab description");
    }
    break;

    case Path::CalibrationImageDir:
    {
        CheckPath(hv_program_params_.hv_calibration_images_dir, "Calibration image dir");
    }
    break;

    case Path::ReconstructionImageDir:
    {
        CheckPath(hv_program_params_.hv_reconstruction_image_data_path, "Reconstruction image dir");
    }
    break;

    case Path::ReconstructionOutputDir:
    {
        CheckPath(hv_program_params_.hv_reconstruction_output_dir, "Reconstruction output dir");
    }
    break;

    case Path::CameraParameters:
    {
        CheckPath(hv_program_params_.hv_poses_dir + "/camera_parameters.dat", "CameraParameters");
    }
    break;

    case Path::CameraPose:
    {
        CheckPath(hv_program_params_.hv_poses_dir + "/camera_pose.dat", "CameraPose");
    }
    break;

    case Path::LightPlanePose:
    {
        CheckPath(hv_program_params_.hv_poses_dir + "/light_plane_pose.dat", "LightPlanePose");
    }
    break;

    case Path::MovementPose:
    {
        CheckPath(hv_program_params_.hv_poses_dir + "/movement_pose.dat", "MovementPose");
    }
    break;

    case Path::All:
    default:
    {
        CheckPath(hv_program_params_.hv_poses_dir, "Poses dir");
        CheckPath(hv_program_params_.hv_caltab_description_path, "Caltab description");
        CheckPath(hv_program_params_.hv_calibration_images_dir, "Calibration image dir");
        CheckPath(hv_program_params_.hv_reconstruction_image_data_path, "Reconstruction image dir");
        CheckPath(hv_program_params_.hv_reconstruction_output_dir, "Reconstruction output dir");
        CheckPath(hv_program_params_.hv_poses_dir + "/camera_parameters.dat", "CameraParameters");
        CheckPath(hv_program_params_.hv_poses_dir + "/camera_pose.dat", "CameraPose");
        CheckPath(hv_program_params_.hv_poses_dir + "/light_plane_pose.dat", "LightPlanePose");
        CheckPath(hv_program_params_.hv_poses_dir + "/movement_pose.dat", "MovementPose");
    }
    break;
    }
}

void SystemParams::initFinished()
{
    auto safe_copy = [](char *dest, const char *src, size_t N)
    {
        if (!src)
        {
            dest[0] = '\0';
            AppController::instance().getLogger().log("Error when copy", Logger::LogLevel::Error);
        }
        else
        {
            std::strncpy(dest, src, N - 1);
            dest[N - 1] = '\0';
        }
    };

    ParamsInitEvent event;

    event.params.disable = false;
    safe_copy(event.params.base_dir, this->base_dir_.string().c_str(), sizeof(event.params.base_dir));
    safe_copy(event.params.config_file_path, this->config_path_.c_str(), sizeof(event.params.config_file_path));
    safe_copy(event.params.poses_dir, this->hv_program_params_.hv_poses_dir[0].S().Text(), sizeof(event.params.poses_dir));

    safe_copy(event.params.calibration_images_dir, this->hv_program_params_.hv_calibration_images_dir[0].S().Text(), sizeof(event.params.calibration_images_dir));
    safe_copy(event.params.caltab_description, this->hv_program_params_.hv_caltab_description_path[0].S().Text(), sizeof(event.params.caltab_description));
    event.params.calibration_image_count = this->hv_program_params_.hv_calibration_image_count[0].I();
    event.params.calibration_min_threshold = this->hv_program_params_.hv_calibration_min_threshold[0].I();
    event.params.calibration_step_count = this->hv_program_params_.hv_calibration_step_count[0].I();
    event.params.calibration_thickness = this->hv_program_params_.hv_calibration_thickness[0].D();
    event.params.calibration_max_light_plane_error = this->hv_program_params_.hv_calibration_max_light_plane_error[0].D();

    safe_copy(event.params.reconstruction_image_data, this->hv_program_params_.hv_reconstruction_image_data_path[0].S().Text(), sizeof(event.params.reconstruction_image_data));
    safe_copy(event.params.reconstruction_output_cloud_dir, this->hv_program_params_.hv_reconstruction_output_dir[0].S().Text(), sizeof(event.params.reconstruction_output_cloud_dir));
    event.params.reconstruction_min_threshold = this->hv_program_params_.hv_reconstruction_min_threshold[0].I();
    event.params.reconstruction_roi_row1 = this->hv_program_params_.hv_reconstruction_roi_row1[0].I();
    event.params.reconstruction_roi_col1 = this->hv_program_params_.hv_reconstruction_roi_col1[0].I();
    event.params.reconstruction_roi_row2 = this->hv_program_params_.hv_reconstruction_roi_row2[0].I();
    event.params.reconstruction_roi_col2 = this->hv_program_params_.hv_reconstruction_roi_col2[0].I();

    event.params.save_cloud_flag = this->save_cloud_flag_;
    event.params.set_roi_flag = this->set_roi_flag_;
    event.params.select_entire_frame = this->select_entire_frame_;

    AppController::instance().publish("params_init_finished", std::make_any<ParamsInitEvent>(event));
}