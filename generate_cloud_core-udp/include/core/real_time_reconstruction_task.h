#pragma once

#include "task.h"

#include <libssh/libssh.h>
#include <libssh/sftp.h>

#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>

class RealTimeReconstructionTask : public Task
{
public:
    RealTimeReconstructionTask(std::string id);
    ~RealTimeReconstructionTask();
      void stop() override;        //2026.3.16 add
private:
    void run() override;
    void producerLoop();
    void consumerLoop();

    std::queue<std::string> ready_files_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;

    bool connect();
    void disconnect();
    std::vector<std::string> listRemoteDirectories();
    bool downloadFile(const std::string& remote_path, const std::string& local_path);
    bool decompressZst(const std::string& zst_path, const std::string& out_path);
    void reconstructionSingleFile(const std::string& bil_path);

    //test
    void producerLoopLocalTest();
    bool isFileReadyToDownload(const std::string& remote_path);
    std::string last_processed_timestamp_;

    ssh_session session_ = nullptr;
    sftp_session sftp_ = nullptr;

    std::thread producer_thread_;
    std::thread consumer_thread_;

};