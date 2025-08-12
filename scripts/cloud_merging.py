# Import project path
import os
import sys
from unittest import result

project_root = os.path.dirname("/data/project/GenerateCloud/")
sys.path.append(project_root)

import numpy as np
import open3d as o3d

from utils.config_loader import load_config_json


if __name__ == "__main__":
    config = load_config_json("config.json")

    cloud_dir = config["reconstruction"]["outputCloudsDir"]

    pcd = o3d.io.read_point_cloud(cloud_dir + "/2025-04-20_10-21-07.ply")
    pcd_down = pcd.voxel_down_sample(5.0)
    pcd_down.estimate_normals(search_param=o3d.geometry.KDTreeSearchParamHybrid(radius=5.0 * 2, max_nn=30))
    accumulated_transformation = np.eye(4)

    current_pcd = o3d.io.read_point_cloud(cloud_dir + "/2025-04-20_1    0-22-07.ply")
    current_pcd_down = current_pcd.voxel_down_sample(5.0)
    current_pcd_down.estimate_normals(search_param=o3d.geometry.KDTreeSearchParamHybrid(radius=5.0 * 2, max_nn=30))

    result_icp = o3d.pipelines.registration.registration_icp(
        current_pcd_down,
        pcd_down,
        10.0,
        accumulated_transformation,
        o3d.pipelines.registration.TransformationEstimationPointToPlane(),
    )

    transformation = result_icp.transformation
    current_pcd.transform(transformation)
