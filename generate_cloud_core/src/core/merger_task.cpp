#include "merger_task.h"

#include <pcl-1.12/pcl/io/ply_io.h>

MergerTask::MergerTask(std::string id) : Task(id)
{
}

void MergerTask::run()
{
    float voxel_size = 5.0f;
}
