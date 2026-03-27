#pragma once

#include <string>
#include <vector>

#include "task.h"

class MergerTask : public Task
{
public:
    MergerTask(std::string id);

private:
    void run() override;

    float voxel_size_;
    float sac_min_sample_dist_;
    float sac_max_corr_dist_;
    int sac_max_iterations_;
    float icp_max_corr_dist_;

    std::vector<std::string> files;
};
