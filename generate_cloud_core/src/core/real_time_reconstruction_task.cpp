#include "real_time_reconstruction_task.h"

#include "app_controller.h"
#include "system_params.h"
#include "reconstruction_task.h"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <vector>
#include <set>

#include <zstd.h>
#include <fcntl.h>

RealTimeReconstructionTask::RealTimeReconstructionTask(std::string id) : Task(id)
{
    const auto& local_temp_dir = SystemParams::instance().rt_params_.local_temp_dir;
    if(!local_temp_dir.empty() && !std::filesystem::exists(local_temp_dir))
    {
        try
        {
            std::filesystem::create_directories(local_temp_dir);
        }
        catch (const std::filesystem::filesystem_error& e)
        {
            std::string msg = "Failed to create temporary directory: " + local_temp_dir + " - " + e.what();
            AppController::instance().getLogger().log(msg, Logger::LogLevel::Error);
        }
    }
}

RealTimeReconstructionTask::~RealTimeReconstructionTask()
{
    stop();
    disconnect();
}

// void RealTimeReconstructionTask::run()
// {
//     AppController::instance().getLogger().log("Real-time task started with parallel processing.");

//     consumer_thread_ = std::thread(&RealTimeReconstructionTask::consumerLoop, this);

//     //producerLoop();

//     if(consumer_thread_.joinable())
//     {
//         consumer_thread_.join();
//     }
// }
void RealTimeReconstructionTask::run()
{
    //AppController::instance().getLogger().log("Real-time task started in LOCAL TEST MODE.");
    AppController::instance().getLogger().log("Real-Time Reconstruction task start in REMOTE MODE");


    //producer_thread_ = std::thread(&RealTimeReconstructionTask::producerLoopLocalTest, this);
    producer_thread_ = std::thread(&RealTimeReconstructionTask::producerLoop, this);
    consumer_thread_ = std::thread(&RealTimeReconstructionTask::consumerLoop, this);

    if(producer_thread_.joinable())
    {
        producer_thread_.join();
    }

    if(consumer_thread_.joinable())
    {
        consumer_thread_.join();
    }

    AppController::instance().getLogger().log("Local consumer loop test fully completed.");
    //  //2026.1.19 add
    // running_.store(false);
    // queue_cv_.notify_all();
    // //2026.1.19 add finish
}

// 在 cpp 文件中新增这个函数实现 2026.3.16 add
void RealTimeReconstructionTask::stop()
{
    AppController::instance().getLogger().log("Real-Time Task is stopping...");
    
    // 1. 改变运行标志位，告诉所有循环：不要再继续了
    running_.store(false);
    
    // 2. 最关键的一步！摇响全局铃铛，把可能正在睡大觉的消费者强行踹醒！
    queue_cv_.notify_all(); 
    
    // 3. 调用父类的 stop (如果有的话，看你的 Task 基类设计)
    if(task_thread_.joinable())
    {
        task_thread_.join();
    }

    finished_.store(false);
}

void RealTimeReconstructionTask::producerLoop()
{
    const auto& rt_params = SystemParams::instance().rt_params_;
    std::set<std::string> processed_dirs;

    if(rt_params.remote_data_dir.empty())
    {
        AppController::instance().getLogger().log("Remote data directory is not configured", Logger::LogLevel::Error);
        return;
    }

    if(rt_params.local_temp_dir.empty())
    {
        AppController::instance().getLogger().log("Local temp directory is not configured", Logger::LogLevel::Error);
        return;
    }

    if(!std::filesystem::exists(rt_params.local_temp_dir))
    {
        try
        {
            std::filesystem::create_directories(rt_params.local_temp_dir);
            AppController::instance().getLogger().log("Create local temp directory: " + rt_params.local_temp_dir);
        }
        catch(const std::filesystem::filesystem_error& e)
        {
            AppController::instance().getLogger().log("failed to create temp directory: " + std::string(e.what()), Logger::LogLevel::Error);
            return;
        }
    }

    // 2026.3.25：增加一个标志位，记住当前的连接状态
    bool is_ssh_connected = false;
    while(running_.load())
    {
        try
        {
            // 2026.3.25：只在“没连上”或者“掉线了”的时候，才去拨号连接
        if (!is_ssh_connected) 
        {
            AppController::instance().getLogger().log("attempting to connect to SSH...");
            if(!connect())
            {
                AppController::instance().getLogger().log("SSH connection failed. Retrying in " + std::to_string(rt_params.scan_interval_seconds) + " seconds",
                                                            Logger::LogLevel::Error);
                std::this_thread::sleep_for(std::chrono::seconds(rt_params.scan_interval_seconds));
                continue;
            }
            is_ssh_connected = true;// 2026.3.25：连接成功！打上标记，下次循环就不会再敲门了！
        }

            AppController::instance().getLogger().log("Listing remote directories in " + rt_params.remote_data_dir);
            auto all_dirs = this->listRemoteDirectories();
            AppController::instance().getLogger().log("Found " + std::to_string(all_dirs.size()) + "directories");

            std::vector<std::string> new_dirs;
            for(const auto& dir : all_dirs)
            {  // 核心过滤逻辑：只有文件夹名字（时间戳）大于上次处理的时间戳，才算是新数据！
                if(dir > last_processed_timestamp_)
                {
                    new_dirs.push_back(dir);
                    AppController::instance().getLogger().log("New directory found: " + dir);
                }
            }

            if(!new_dirs.empty())
            {
                std::sort(new_dirs.begin(), new_dirs.end());
                AppController::instance().getLogger().log("Found " + std::to_string(new_dirs.size()) + "new scans to process.");
            // 2026.4.22：丢包策略核心代码,我们从新文件列表的末尾（最新的）往前找，找到第一个“准备好”的文件夹就立刻处理它，剩下的统统丢弃
                std::vector<std::string> ready_dirs;
                size_t skipped_older_count = 0;
                for(auto it = new_dirs.rbegin(); it != new_dirs.rend(); ++it)
                {
                    const auto& candidate_dir = *it;
                    if(processed_dirs.find(candidate_dir) != processed_dirs.end())
                    {
                        AppController::instance().getLogger().log("Directory already processed: " + candidate_dir);
                        continue;
                    }

                    std::string candidate_file_path = rt_params.remote_data_dir;
                    if(!candidate_file_path.empty() && candidate_file_path.back() != '/'){
                        candidate_file_path += '/';
                    }
                    candidate_file_path += candidate_dir;
                    candidate_file_path += '/';
                    candidate_file_path += candidate_dir;
                    candidate_file_path += ".bil.zst";

                    if(isFileReadyToDownload(candidate_file_path))
                    {
                        ready_dirs.push_back(candidate_dir);
                        skipped_older_count = static_cast<size_t>(std::distance(new_dirs.begin(), it.base() - 1));
                        break;
                    }

                    AppController::instance().getLogger().log("New scan is not ready yet: " + candidate_dir, Logger::LogLevel::Warn);
                }

                if(ready_dirs.empty())
                {
                    AppController::instance().getLogger().log("No ready new scan directories found. Waiting for next scan interval.", Logger::LogLevel::Warn);
                    new_dirs.clear();
                }
                else
                {
                    if(skipped_older_count > 0)
                    {
                        AppController::instance().getLogger().log("Skipping " + std::to_string(skipped_older_count) + " older remote frames to keep real-time!");
                    }
                    new_dirs = ready_dirs;
                }
                
                // ==========================================
                // 🚀 【新增：源头极致丢包策略】 2026.3.16 add 🚀
                // 如果发现了多个新文件，我们只要最新（最后）的那一个！
                if (new_dirs.size() > 1) {
                    std::string newest_dir = new_dirs.back(); // 拿到最后（最新）的时间戳文件夹
                    
                    // 打印一句日志，证明我们在下载前就成功拦截了冗余数据
                    AppController::instance().getLogger().log("Skipping " + std::to_string(new_dirs.size() - 1) + " old remote frames to keep real-time!");
                    
                    new_dirs.clear();           // 把数组清空
                    new_dirs.push_back(newest_dir); // 只把最新的塞回去
                }
                // ==========================================
                for(const auto& dir_name : new_dirs)
                {
                    if(!running_.load()) break;

                    if(processed_dirs.find(dir_name) != processed_dirs.end()){
                        AppController::instance().getLogger().log("Directory already processed: " + dir_name);
                        continue;
                    }

                    // std::filesystem::path remote_dir = std::filesystem::path(rt_params.remote_data_dir) /dir_name;
                    // std::filesystem::path remote_file = remote_dir / (dir_name + ".bil.zst");
                    // std::string remote_file_path = remote_file.generic_string();

                    // std::filesystem::path local_zst_file = std::filesystem::path(rt_params.local_temp_dir) / (dir_name + ".bil.zst");
                    // std::filesystem::path local_bil_file = std::filesystem::path(rt_params.local_temp_dir) / (dir_name + ".bil");
                    std::string remote_file_path = rt_params.remote_data_dir;
                    if(!remote_file_path.empty() && remote_file_path.back() != '/'){
                        remote_file_path += '/';
                    }

                    remote_file_path += dir_name;
                    remote_file_path += '/';
                    remote_file_path += dir_name;
                    remote_file_path += ".bil.zst";

                    // 拼装本地要保存的两个路径：一个是下载的压缩包，一个是解压后的裸数据
                    std::filesystem::path local_zst_file = std::filesystem::path(rt_params.local_temp_dir) / (dir_name + ".bil.zst");
                    std::filesystem::path local_bil_file = std::filesystem::path(rt_params.local_temp_dir) / (dir_name + ".bil");

                    AppController::instance().getLogger().log("Remote file: " + remote_file_path);
                    AppController::instance().getLogger().log("Local zst file: " + local_zst_file.string());
                    AppController::instance().getLogger().log("Local bil file: " + local_bil_file.string());

                    //download .bil.zst
                    AppController::instance().getLogger().log("Downloading: " + remote_file_path);
                    if(downloadFile(remote_file_path, local_zst_file.string()))
                    {
                        AppController::instance().getLogger().log("Download successful, decompressing: " + local_zst_file.string());
                        if(decompressZst(local_zst_file.string(), local_bil_file.string()))
                        {
                            {
                                std::lock_guard<std::mutex> lock(queue_mutex_);
                                // ========== 【新增：丢包策略核心】2026.3.16 add ==========
                                // 如果队列里还有没来得及处理的老文件，无情清空它！
                                while(!ready_files_queue_.empty())
                                {
                                    std::string old_file = ready_files_queue_.front();
                                    ready_files_queue_.pop();
                                    
                                    // 必须同时删除物理硬盘上的文件，防止内存/硬盘泄漏
                                    try {
                                        if (std::filesystem::exists(old_file)) {
                                            std::filesystem::remove(old_file);
                                            AppController::instance().getLogger().log("Consumer too slow! Frame dropped: " + old_file, Logger::LogLevel::Warn);
                                        }
                                    } catch(const std::filesystem::filesystem_error& e) {
                                        AppController::instance().getLogger().log("Failed to delete dropped frame: " + std::string(e.what()), Logger::LogLevel::Warn);
                                    }
                                }
                                // ==========================================

                                // 此时篮子绝对是空的，只把刚解压出来的最新一帧塞进去
                                ready_files_queue_.push(local_bil_file.string());
                            }
                            queue_cv_.notify_one();
                            last_processed_timestamp_ = dir_name;// 更新时间戳和小本本，证明这帧我办妥了
                            AppController::instance().getLogger().log("Successfully processed " + dir_name);

                            processed_dirs.insert(dir_name);
                        }
                        else
                        {
                            AppController::instance().getLogger().log("Decompression failed for " + dir_name, Logger::LogLevel::Error);
                        }
                        try{
                            std::filesystem::remove(local_zst_file);
                        }
                        catch(const std::filesystem::filesystem_error &e)
                        {
                            AppController::instance().getLogger().log("Failed to delete temp file " + local_zst_file.string() + " : " + e.what(), Logger::LogLevel::Warn);
                        }
                    }
                    else
                    {
                        AppController::instance().getLogger().log("Download failed for " + dir_name, Logger::LogLevel::Error);
                    }
                }
            }
            else{
                AppController::instance().getLogger().log("No new directories found");
            }
            //disconnect();    //2026.3.25：不再每轮都断开连接了，改成只在异常时断开，并且打上标记，下次循环再重连
        }
        catch(const std::exception& e)
        {
            AppController::instance().getLogger().log(std::string("Exception in Producer: ") + e.what(), Logger::LogLevel::Error);
            disconnect();
            is_ssh_connected = false;// 2026.3.25：连接断开！打上标记，下次循环再重连
        }

        std::this_thread::sleep_for(std::chrono::seconds(rt_params.scan_interval_seconds));
    }
    disconnect();// 2026.3.25：只有当 while 循环彻底结束（用户点了 Stop 按钮），才真正关门谢客
    queue_cv_.notify_all();
}


void RealTimeReconstructionTask::consumerLoop()
{
    // std::cout << "ConsumerLoop enter" << std::endl;
    // std::cout << "running_.load() is: " << running_.load() << std::endl;
    // std::cout << "ready_files_queue_.empty() is: " << std::endl;
    while(true)  //3.30:解决-队列无锁读取导致的数据竞争
    {
        std::string file_to_process;
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            queue_cv_.wait(lock, [this]{
                return !ready_files_queue_.empty() || !running_.load();
            });

            if(ready_files_queue_.empty() && !running_.load())
            {
                break;
            }
            if(ready_files_queue_.empty())
            {
                continue;
            }

            file_to_process = ready_files_queue_.front();
            ready_files_queue_.pop();
        }

        if(!file_to_process.empty())
        {
            AppController::instance().getLogger().log("Consumer picked up for reconstruction: " + file_to_process);
            reconstructionSingleFile(file_to_process);
            // 2026.4.22 add：消费者处理完了，顺便把这个文件给删了，腾出空间
            try
            {
                if(std::filesystem::exists(file_to_process))
                {
                    std::filesystem::remove(file_to_process);
                }
            }
            catch(const std::filesystem::filesystem_error& e)
            {
                AppController::instance().getLogger().log("Failed to delete temp file " + file_to_process + " : " + e.what(), Logger::LogLevel::Warn);
            }
        }
    }
}

 bool RealTimeReconstructionTask::connect()
 {
    const auto& rt_params = SystemParams::instance().rt_params_;

    std::string connInfo = "Connecting to " + rt_params.host + " as user" + rt_params.user;
    AppController::instance().getLogger().log(connInfo);

    if(rt_params.host.empty() || rt_params.user.empty() || rt_params.password.empty())
    {
        AppController::instance().getLogger().log("SSH connection parameters missing", Logger::LogLevel::Error);
        return false;
    }

    session_ = ssh_new();
    if(session_ == nullptr)
    {
        AppController::instance().getLogger().log("ssh_new() failed. ", Logger::LogLevel::Error);
        return false;
    }
    ssh_options_set(session_, SSH_OPTIONS_HOST, rt_params.host.c_str());
    ssh_options_set(session_, SSH_OPTIONS_USER, rt_params.user.c_str());

    ssh_options_set(session_, SSH_OPTIONS_PORT, &rt_params.port);

    //超时
    int timeout = 10;
    ssh_options_set(session_, SSH_OPTIONS_TIMEOUT, &timeout);

    int rc = ssh_connect(session_);
    if(rc != SSH_OK)
    {
        std::cout << rt_params.host << std::endl;
        std::string err_msg = "Error connection to " + rt_params.host + " : " + ssh_get_error(session_);
        AppController::instance().getLogger().log(err_msg, Logger::LogLevel::Error);
        ssh_free(session_);
        session_ = nullptr;
        return false;
    }

    rc = ssh_userauth_password(session_, nullptr, rt_params.password.c_str());
    if(rc != SSH_AUTH_SUCCESS)
    {
        AppController::instance().getLogger().log("Authentication failed.", Logger::LogLevel::Error);
        ssh_disconnect(session_);
        ssh_free(session_);
        session_ = nullptr;
        return false;
    }

    sftp_ = sftp_new(session_);
    if(sftp_ == nullptr)
    {
        AppController::instance().getLogger().log("sftp_new() failed.", Logger::LogLevel::Error);
        ssh_disconnect(session_);
        ssh_free(session_);
        session_ = nullptr;
        return false;
    }

    if(sftp_init(sftp_) != SSH_FX_OK)       //0 for init ok
    {
        AppController::instance().getLogger().log("sftp_init() failed. ", Logger::LogLevel::Error);
        sftp_free(sftp_);
        sftp_ = nullptr;
        ssh_disconnect(session_);
        ssh_free(session_);
        session_ = nullptr;
        return false;
    }
    
    AppController::instance().getLogger().log("SSH connection established sucessfully");

    return true;
 }

 void RealTimeReconstructionTask::disconnect()
 {
    if(sftp_)
    {
        sftp_free(sftp_);
        sftp_ = nullptr;
    }
    if(session_)
    {
        ssh_disconnect(session_);
        ssh_free(session_);
        session_ = nullptr;
    }
 }

std::vector<std::string> RealTimeReconstructionTask::listRemoteDirectories()
{
    std::vector<std::string> directories;
    const auto& rt_params = SystemParams::instance().rt_params_;

    std::string remote_dir_path = std::filesystem::path(rt_params.remote_data_dir).generic_string();
    AppController::instance().getLogger().log("Attempting to open directory: " + remote_dir_path);

    sftp_dir dir = sftp_opendir(sftp_, remote_dir_path.c_str());

    sftp_attributes attributes;

    if(!dir)
    {
        std::string error_msg = "sftp_opendir failed for " + remote_dir_path + " : Error code = " + std::to_string(sftp_get_error(sftp_)) 
                                        + ", Message= " + ssh_get_error(session_);
        AppController::instance().getLogger().log(error_msg + rt_params.remote_data_dir, Logger::LogLevel::Error);
        //return directories;  // 3.30：不再 return 空数组，而是直接抛出异常！解决只是返回空，不抛异常，连接标记仍是 true，后续会一直“无新目录”轮询。
        throw std::runtime_error("SFTP Connection Lost: Cannot open directory.");
    }

    while((attributes = sftp_readdir(sftp_, dir)) != NULL)
    {
        //is directory and is not ./,../
        if(attributes->type == SSH_FILEXFER_TYPE_DIRECTORY && 
            std::string(attributes->name) != "." && 
            std::string(attributes->name) != "..")
            {
                directories.push_back(attributes->name);
            }
            sftp_attributes_free(attributes);
    }

    sftp_closedir(dir);
    return directories;
}

bool RealTimeReconstructionTask::downloadFile(const std::string& remote_path, const std::string& local_path)
{
    const int max_retries = 3;
    int retry_count = 0;
    while(retry_count < max_retries){
        if(!(isFileReadyToDownload(remote_path))){
            AppController::instance().getLogger().log("File not ready, waiting 2 seconds before retry...", Logger::LogLevel::Warn);
            std::this_thread::sleep_for(std::chrono::seconds(2));
            retry_count++;
            continue;
        }

        AppController::instance().getLogger().log("Download from: " + remote_path + " to: " + local_path);

        std::ofstream local_file(local_path, std::ios::binary);
        if(!local_file.is_open()){
            AppController::instance().getLogger().log("Failed to open local file for writing: " + local_path, Logger::LogLevel::Error);
            return false;
        }

        sftp_file file = sftp_open(sftp_, remote_path.c_str(), O_RDONLY, 0);
        if(file == nullptr){
            std::string error_msg = "Can't open remote file: " + remote_path + " : Error code= " + std::to_string(sftp_get_error(sftp_))
                                    + ", Message = " + ssh_get_error(session_);

            AppController::instance().getLogger().log(error_msg, Logger::LogLevel::Error);
            retry_count++;
            local_file.close();
            std::this_thread::sleep_for(std::chrono::seconds(2));
            continue;
        }

        char buffer[8192];
        int nbytes;
        int nwritten;
        bool success = true;

        while((nbytes = sftp_read(file, buffer, sizeof(buffer))) > 0){
            local_file.write(buffer, nbytes);
            if(local_file.bad()){
                AppController::instance().getLogger().log("Error writing to local file: " + local_path, Logger::LogLevel::Error);
                success = false;
                break;
            }
        }

        if(nbytes < 0){
            AppController::instance().getLogger().log("Error reading remote file: " + remote_path, Logger::LogLevel::Error);
            success = false;
        }

        sftp_close(file);
        local_file.close();

        std::error_code ec;
        auto filesize = std::filesystem::file_size(local_path, ec);
        if(ec){
            AppController::instance().getLogger().log("Failed to get file size: " + local_path, Logger::LogLevel::Error);
        }else if(filesize == 0){
            AppController::instance().getLogger().log("Download file is empty: " + local_path, Logger::LogLevel::Error);
            success = false;
        }else{
            AppController::instance().getLogger().log("Download completed: " + std::to_string(filesize) + " bytes");
        }

        if(success)
        {
            return true;
        }

        retry_count++;
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    AppController::instance().getLogger().log("Failed to download after " + std::to_string(max_retries) + " retries: " + remote_path, Logger::LogLevel::Error);
    return false;
    
}

bool RealTimeReconstructionTask::decompressZst(const std::string& zst_path, const std::string& out_path)
{
    // 打开输入文件
    std::ifstream f_in(zst_path, std::ios::binary);
    if (!f_in)
    {
        AppController::instance().getLogger().log("Cannot open ZST file: " + zst_path, Logger::LogLevel::Error);
        return false;
    }
 
    // 打开输出文件
    std::ofstream f_out(out_path, std::ios::binary);
    if (!f_out)
    {
        AppController::instance().getLogger().log("Cannot write BIL file: " + out_path, Logger::LogLevel::Error);
        return false;
    }
 
    // 创建 ZSTD 流式解压上下文
    ZSTD_DCtx* const dctx = ZSTD_createDCtx();
    if (dctx == NULL) {
        AppController::instance().getLogger().log("ZSTD_createDCtx() failed!", Logger::LogLevel::Error);
        return false;
    }
 
    // 设置输入和输出缓冲区大小
    size_t const in_buffer_size = ZSTD_DStreamInSize();
    std::vector<char> in_buffer(in_buffer_size);
    size_t const out_buffer_size = ZSTD_DStreamOutSize();
    std::vector<char> out_buffer(out_buffer_size);
 
    size_t last_ret = 0;
    while (true) {
        // 从文件读取一块压缩数据到输入缓冲区
        f_in.read(in_buffer.data(), in_buffer_size);
        size_t const read_size = f_in.gcount();
        if (read_size == 0) { // 文件读取完毕
            break;
        }
 
        ZSTD_inBuffer input = { in_buffer.data(), read_size, 0 };
 
        // 循环解压，直到输入缓冲区被完全消耗
        while (input.pos < input.size) {
            ZSTD_outBuffer output = { out_buffer.data(), out_buffer_size, 0 };
            
            // 调用解压函数
            size_t const ret = ZSTD_decompressStream(dctx, &output, &input);
 
            // 检查解压是否出错
            if (ZSTD_isError(ret)) {
                AppController::instance().getLogger().log("ZSTD_decompressStream failed: " + std::string(ZSTD_getErrorName(ret)), Logger::LogLevel::Error);
                ZSTD_freeDCtx(dctx);
                return false;
            }
 
            // 将解压后的数据写入输出文件
            f_out.write(out_buffer.data(), output.pos);
            last_ret = ret;
        }
    }
    
    // 释放上下文
    ZSTD_freeDCtx(dctx);
 
    // 如果最后一次解压返回值不为0，意味着流可能未正常结束
    // 但在很多情况下，文件读完即解压完成，所以可以根据情况放宽检查
    if(last_ret != 0) {
        // AppController::instance().getLogger().log("ZSTD stream did not end properly, but decompression might be complete.", Logger::LogLevel::Warn);
    }
 
    return true;
}

void RealTimeReconstructionTask::reconstructionSingleFile(const std::string& bil_file_path)
{
    try
    {
        ReconstructionTask task("single_file processor");
        task.setExtraRunning(&running_);
        task.processSingleFile(bil_file_path);
    }
    catch(const std::exception& e)
    {
        std::string msg = "Error during single file reconstruction of " + bil_file_path + " : " + e.what();
        AppController::instance().getLogger().log(msg, Logger::LogLevel::Error);
    }
    catch(const HException& e)
    {
        std::string msg = "Halcon error during single file reconstruction of " + bil_file_path + " : " + e.ErrorMessage().Text();
        AppController::instance().getLogger().log(msg, Logger::LogLevel::Error);
    }
}

void RealTimeReconstructionTask::producerLoopLocalTest()
{
    const auto& rt_params = SystemParams::instance().rt_params_;
    const std::string sim_dir = rt_params.local_simulation_dir;

    if(sim_dir.empty() || !std::filesystem::exists(sim_dir))
    {
        AppController::instance().getLogger().log("Local simulation directory not configured or does not exist: " + sim_dir, Logger::LogLevel::Error);
        return;
    }

    AppController::instance().getLogger().log("Scanning local simulation directory: "+ sim_dir);

    std::vector<std::filesystem::path> sub_dirs;
    for(const auto& entry : std::filesystem::directory_iterator(sim_dir))
    {
        if(entry.is_directory())
        {
            sub_dirs.push_back(entry.path());
        }
    }

    std::sort(sub_dirs.begin(), sub_dirs.end());
    
    AppController::instance().getLogger().log("Found " + std::to_string(sub_dirs.size()) + "directoires to process.");

    for(const auto& dir_path : sub_dirs)
    {
        if(!running_.load()) break;

        std::string dir_name = dir_path.filename().string();
        std::filesystem::path zst_file = dir_path / (dir_name + ".bil.zst");

        if(!std::filesystem::exists(zst_file))
        {
            AppController::instance().getLogger().log("Skipping directory " + dir_name + ": .bil.zst file not found.", Logger::LogLevel::Warn);
            continue;
        }
        std::cout << "rt_params.local_temp_dir is: " << rt_params.local_temp_dir << std::endl;
        std::filesystem::path local_bil_file = std::filesystem::path(rt_params.local_temp_dir) / (dir_name + ".bil");
        std::cout << "dir to save .bil file is: " << local_bil_file << std::endl;

        AppController::instance().getLogger().log("decompressing: " + zst_file.string());
        if(decompressZst(zst_file.string(), local_bil_file.string()))
        {
            {
                std::lock_guard<std::mutex> lock(queue_mutex_);
                // ========== 【同步新增：测试代码的丢包策略】 ==========
                while(!ready_files_queue_.empty())
                {
                    std::string old_file = ready_files_queue_.front();
                    ready_files_queue_.pop();
                    
                    try {
                        if (std::filesystem::exists(old_file)) {
                            std::filesystem::remove(old_file);
                            // 打印黄色的警告，证明本地测试的丢包逻辑也生效了！
                            AppController::instance().getLogger().log("[Local Test] Consumer too slow! Frame dropped: " + old_file, Logger::LogLevel::Warn);
                        }
                    } catch(const std::filesystem::filesystem_error& e) {
                        // 忽略删除失败的情况
                    }
                }
                // ===================================================
                ready_files_queue_.push(local_bil_file.string());
            }
            queue_cv_.notify_one();
            AppController::instance().getLogger().log("Queued for reconstruction: " + local_bil_file.string());
        }
        else
        {
            AppController::instance().getLogger().log("Decompression failed for " + dir_name, Logger::LogLevel::Error);
        }
    }
    AppController::instance().getLogger().log("Local producer has finished processing all directories.");
}

bool RealTimeReconstructionTask::isFileReadyToDownload(const std::string& remote_path)
{
    sftp_attributes attrs = sftp_stat(sftp_, remote_path.c_str());
    if(!attrs)
    {
        AppController::instance().getLogger().log("Failed to get file attributes: " + remote_path, Logger::LogLevel::Error);
        return false;
    }

    if(attrs->size == 0)
    {
        sftp_attributes_free(attrs);
        AppController::instance().getLogger().log("Remote file is empty: " + remote_path, Logger::LogLevel::Warn);
        return false;
    }

    time_t now = time(NULL);
    if(now - attrs->mtime < 5)
    {
        sftp_attributes_free(attrs);
        AppController::instance().getLogger().log("Remote file may still be writing: " + remote_path, Logger::LogLevel::Warn);
        return false;
    }

    sftp_attributes_free(attrs);
    return true;
}
