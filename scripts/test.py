# Import project path
import os
import sys

project_root = os.path.dirname("/data/project/GenerateCloud/")
sys.path.append(project_root)

from utils.binary_reader import BinaryReader
from utils.config_load import load_config_json
from utils.image_saver import ImageSaver
from utils.image_stabilizer import ImageStabilizer
from utils.ins_reader import InsReader
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
        path=config["reconstruction"]["imageDataDir"],
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
            print(f"New file opened: {filename}")
            image_saver = ImageSaver(
                "/data/project/GenerateCloud/data/imgs/corrected/"
                + os.path.basename(filename).split(".")[0]
                + ".avi",
                config["global"]["imageWidth"],
                config["global"]["imageHeight"],
                config["global"]["fps"],
            )

        time = log_time_mapper.get_estimated_timestamp(frame_idx)
        pose = ins_reader.get_interpolated_pose(time)
        roll, pitch, yaw = pose["roll"], pose["pitch"], pose["yaw"]
        print(
            f"Idx: {frame_idx}, Time: {time}, Roll: {roll}, Pitch: {pitch}, Yaw: {yaw}"
        )
        corrected_frame = img_stabilizer.correct_image(frame, roll, pitch, 0)

        image_saver.write(corrected_frame)

        frame_idx += 1

    binary_reader.close()
