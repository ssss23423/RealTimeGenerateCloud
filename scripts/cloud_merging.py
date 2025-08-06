# Import project path
import os
import sys

project_root = os.path.dirname("/data/project/GenerateCloud/")
sys.path.append(project_root)

import open3d as o3d

from utils.config_loader import load_config_json


if __name__ == "__main__":
    config = load_config_json("config.json")

    cloud_dir = config["reconstruction"]["outputCloudsDir"]

    mesh = o3d.io.read_point_cloud(cloud_dir + "/2025-04-20_10-21-07.ply")
    print(mesh)
