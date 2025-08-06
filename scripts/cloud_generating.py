# Import project path
import os
import sys

project_root = os.path.dirname("/data/project/GenerateCloud/")
sys.path.append(project_root)

import math
import halcon as ha

from utils.config_loader import load_config_json


def calculate_lines_gauss_parameters(max_line_width: int, contrast: int | list):
    """
    :param max_line_width: int
    :param contrast: int | list
    """
    if not isinstance(max_line_width, int):
        raise TypeError("Wrong type of max_line_width")
    if max_line_width <= 0:
        raise ValueError("Wrong value of max_line_width")

    if not (len(contrast) == 1 or len(contrast) == 2):
        raise ValueError("Wrong number of values of contrast")
    if not all(isinstance(c, int) for c in contrast):
        raise TypeError("Wrong type of contrast")

    contrast_high = contrast[0]
    if contrast_high < 0:
        raise ValueError("Wrong value of contrast_high")

    if len(contrast) == 2:
        contrast_low = contrast[1]
    else:
        contrast_low = contrast_high / 3.0

    if contrast_low < 0 or contrast_low > contrast_high:
        raise ValueError("Wrong value of contrast_low")

    sqrt3 = math.sqrt(3.0)
    if max_line_width < sqrt3:
        factor = max_line_width / sqrt3
        contrast_low *= factor
        contrast_high *= factor
        max_line_width = sqrt3

    half_width = max_line_width / 2.0
    sigma = half_width / math.sqrt(3.0)
    help = -2.0 * half_width / (math.sqrt(2 * math.pi) * (sigma**3)) * math.exp(-0.5 * (half_width / sigma) ** 2)
    high = abs(contrast_high * help)
    low = abs(contrast_low * help)

    return sigma, low, high


if __name__ == "__main__":
    config = load_config_json("config.json")
    img_width = config["global"]["imageWidth"]
    img_height = config["global"]["imageHeight"]
    poses_dir = config["reconstruction"]["poses"]
    output_dir = config["reconstruction"]["outputCloudsDir"]

    camera_param = ha.read_cam_par(os.path.join(poses_dir, "camera_parameters.dat"))
    camera_pose = ha.read_pose(os.path.join(poses_dir, "camera_pose.dat"))
    light_plane_pose = ha.read_pose(os.path.join(poses_dir, "light_plane_pose.dat"))
    movement_pose = ha.read_pose(os.path.join(poses_dir, "movement_pose.dat"))

    image_files = ha.list_files(
        "/data/project/GenerateCloud/data/imgs/corrected/2025-04-20_10-22-07",
        ["files", "recursive"],
    )

    ha.set_system("clip_region", "false")
    ho_profile_region = ha.gen_rectangle1(0, 0, img_height - 1, img_width - 1)
    sheet_of_light_model_id = ha.create_sheet_of_light_model(
        ho_profile_region,
        ["min_gray", "num_profiles", "ambiguity_solving"],
        [100, len(image_files), "first"],
    )
    ha.set_sheet_of_light_param(sheet_of_light_model_id, "calibration", "xyz")
    ha.set_sheet_of_light_param(sheet_of_light_model_id, "scale", "mm")
    ha.set_sheet_of_light_param(sheet_of_light_model_id, "camera_parameter", camera_param)
    ha.set_sheet_of_light_param(sheet_of_light_model_id, "camera_pose", camera_pose)
    ha.set_sheet_of_light_param(sheet_of_light_model_id, "lightplane_pose", light_plane_pose)
    ha.set_sheet_of_light_param(sheet_of_light_model_id, "movement_pose", movement_pose)

    # Init gauss
    max_line_width = 20
    sigma, low, high = calculate_lines_gauss_parameters(max_line_width, [50, 10])

    for idx in range(len(image_files)):
        print(idx)
        image = ha.read_image(image_files[idx])

        lines = ha.lines_gauss(image, sigma, low, high, "light", "true", "gaussian", "true")
        line_count = ha.count_obj(lines)
        line = ha.gen_empty_obj()

        for i in range(1, line_count + 1):
            selected_line = ha.select_obj(lines, i)
            row, col = ha.get_contour_xld(selected_line)
            region = ha.gen_region_polygon(row, col)
            line = ha.concat_obj(line, region)

        cleared_image = ha.gen_image_proto(image, 0)
        result = ha.paint_region(line, cleared_image, 255, "fill")

        ha.measure_profile_sheet_of_light(result, sheet_of_light_model_id, [])
        disparity = ha.get_sheet_of_light_result(sheet_of_light_model_id, "disparity")

    object_model_3d_id = ha.get_sheet_of_light_result_object_model_3d(sheet_of_light_model_id)

    pose_trans = list(camera_pose)
    pose_trans[0] *= 1000
    pose_trans[1] *= 1000
    pose_trans[2] *= 1000
    object_model_affine_trans = ha.rigid_trans_object_model_3d(object_model_3d_id, pose_trans)

    ha.write_object_model_3d(
        object_model_affine_trans,
        "ply",
        os.path.join(output_dir, "2025-04-20_10-22-07.ply"),
        [],
        [],
    )
