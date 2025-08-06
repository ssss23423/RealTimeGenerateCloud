# Import project path
import os
import sys

project_root = os.path.dirname("/data/project/GenerateCloud/")
sys.path.append(project_root)

from utils.binary_reader import BinaryReader
from utils.config_loader import load_config_json
from utils.image_saver import ImageSaver
from utils.image_stabilizer import ImageStabilizer
from utils.ins_reader import InsReader
from utils.log import logger
from utils.log_time_mapper import LogTimeMapper


if __name__ == "__main__":
    config = load_config_json("config.json")

    log_time_mapper = LogTimeMapper(config["reconstruction"]["parsedLogData"])

    ins_reader = InsReader(config["reconstruction"]["parsedInsData"])

    img_stabilizer = ImageStabilizer(
        cx=config["camera"]["cx"],
        cy=config["camera"]["cy"],
        sx=config["camera"]["sx"],
        sy=config["camera"]["sy"],
        focus=config["camera"]["focus"],
    )

    binary_reader = BinaryReader(
        path=config["reconstruction"]["imageData"],
        img_width=config["global"]["imageWidth"],
        img_height=config["global"]["imageHeight"],
        file_ext=config["global"]["datatype"],
    )

    # Start from 1
    frame_idx = 1
    while True:
        frame, filename = binary_reader.read_frame()
        if frame is None:
            break

        if filename is not None:
            logger.info(f"New file opened: {filename}")
            filename_no_ext = os.path.basename(filename).split(".")[0]
            save_dir = os.path.join("/data/project/GenerateCloud/data/imgs/corrected/", f"{filename_no_ext}")
            if os.path.exists(save_dir) is False:
                os.makedirs(save_dir)
            image_saver = ImageSaver(
                save_dir,
                config["global"]["imageWidth"],
                config["global"]["imageHeight"],
                config["global"]["fps"],
            )

        est_time, (idx_lower, timestamp_lower), (idx_upper, timestamp_upper) = log_time_mapper.get_estimated_timestamp(
            frame_idx, True
        )
        pose, (ins_time_lower, pose_lower), (ins_time_upper, pose_upper) = ins_reader.get_interpolated_pose(
            est_time, True
        )
        roll, pitch, yaw = pose["roll"], pose["pitch"], pose["yaw"]
        logger.info(
            f"""
Idx: {frame_idx};
Est Time: {est_time}
Pose: {pose}
Lower Bound: idx = {idx_lower}; timestamp = {timestamp_lower}; ins_time = {ins_time_lower}; pose = {pose_lower}
Upper Bound: idx = {idx_upper}; timestamp = {timestamp_upper}; ins_time = {ins_time_upper}; pose = {pose_upper}
----------------------------------------------------------------------------------------------------------------
"""
        )
        corrected_frame = img_stabilizer.correct_image(frame, roll, pitch, 0)

        save_path = os.path.join(save_dir, f"{frame_idx:07}.png")
        image_saver.write(corrected_frame, save_path)

        frame_idx += 1

    binary_reader.close()
