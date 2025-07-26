import os
import cv2
import numpy as np
from natsort import natsorted
import zstandard as zstd

from image_stabilizer import ImageStabilizer


class BilReader:

    def __init__(
        self,
        path,
        width=1280,
        height=800,
        fps=120,
        file_ext=".bil",
        output_ext=".mp4",
        is_correct=False,
    ):
        self.width = width
        self.height = height
        self.frame_size = width * height
        self.fps = fps

        self.path = path
        self.file_ext = file_ext
        self.output_ext = output_ext
        self.files = self._gather_files()

        self.frame_count = 1

        if is_correct is True:
            self.image_stabilizer = ImageStabilizer(
                cx=664.923, cy=411.884, sx=4.68544e-06, sy=4.7e-06, focus=0.0111401
            )
        else:
            self.image_stabilizer = None

    def _gather_files(self):
        if os.path.isfile(self.path):
            return [self.path]
        elif os.path.isdir(self.path):
            all_files = [
                os.path.join(self.path, f)
                for f in os.listdir(self.path)
                if f.endswith(self.file_ext)
            ]
            return natsorted(all_files)
        else:
            raise ValueError(f"Invalid path: {self.path}")

    def zst_convert_to_mp4(self, compressed_path, save_dir):
        file_name = os.path.basename(compressed_path).split(".")[0]
        output_video = os.path.join(save_dir, file_name + ".mp4")

        fourcc = cv2.VideoWriter_fourcc(*"mp4v")
        video_writer = cv2.VideoWriter(
            output_video, fourcc, self.fps, (self.width, self.height), isColor=False
        )

        dctx = zstd.ZstdDecompressor()

        with open(compressed_path, "rb") as compressed_file:
            with dctx.stream_reader(compressed_file) as reader:
                while True:
                    chunk = reader.read(self.frame_size)
                    if len(chunk) < self.frame_size:
                        break

                    img = np.frombuffer(chunk, dtype=np.uint8).reshape(
                        (self.height, self.width)
                    )
                    video_writer.write(img)

        video_writer.release()

    def bil_convert_to_mp4(self, file_path, save_dir):
        file_name = os.path.basename(file_path).split(".")[0]
        output_video = os.path.join(save_dir, file_name, ".mp4")

        fourcc = cv2.VideoWriter_fourcc(*"mp4v")
        video_writer = cv2.VideoWriter(
            output_video, fourcc, self.fps, (self.width, self.height), isColor=False
        )

        with open(file_path, "rb") as f:
            while True:
                data = f.read(self.frame_size)
                if len(data) < self.frame_size:
                    break

                img = np.frombuffer(data, dtype=np.uint8).reshape(
                    (self.height, self.width)
                )
                if img.size < self.frame_size:
                    break

                img = img.reshape(self.height, self.width)
                video_writer.write(img)

        video_writer.release()

    def zst_convert_to_avi(self, compressed_path, save_dir):
        file_name = os.path.basename(compressed_path).split(".")[0]
        output_video = os.path.join(save_dir, file_name + ".avi")

        fourcc = cv2.VideoWriter_fourcc(*"XVID")
        video_writer = cv2.VideoWriter(
            output_video, fourcc, self.fps, (self.width, self.height), isColor=False
        )

        dctx = zstd.ZstdDecompressor()

        with open(compressed_path, "rb") as compressed_file:
            with dctx.stream_reader(compressed_file) as reader:
                while True:
                    chunk = reader.read(self.frame_size)
                    if len(chunk) < self.frame_size:
                        break

                    img = np.frombuffer(chunk, dtype=np.uint8).reshape(
                        self.height, self.width
                    )

                    if self.image_stabilizer is not None:
                        img = self.image_stabilizer.correct_image(
                            img, -5.82275390625, -2.2796630859375, 177.681884765625
                        )

                    video_writer.write(img)

        video_writer.release()

    def bil_convert_to_avi(self, file_path, save_dir):
        file_name = os.path.basename(file_path).split(".")[0]
        output_video = os.path.join(save_dir, file_name, ".avi")

        fourcc = cv2.VideoWriter_fourcc(*"XVID")
        video_writer = cv2.VideoWriter(
            output_video, fourcc, self.fps, (self.width, self.height), isColor=False
        )

        with open(file_path, "rb") as f:
            while True:
                data = f.read(self.frame_size)
                if len(data) < self.frame_size:
                    break

                img = np.frombuffer(data, dtype=np.uint8).reshape(
                    (self.height, self.width)
                )
                if img.size < self.width * self.height:
                    break

                img = img.reshape(self.height, self.width)
                video_writer.write(img)

        video_writer.release()

    def zst_convert_to_png(self, compressed_path, save_dir):
        file_name = os.path.basename(compressed_path).split(".")[0]
        save_sub_dir = os.path.join(save_dir, file_name)
        if os.path.exists(save_sub_dir) is False:
            os.makedirs(save_sub_dir)

        dctx = zstd.ZstdDecompressor()

        with open(compressed_path, "rb") as compressed_file:
            with dctx.stream_reader(compressed_file) as reader:
                while True:
                    save_path = os.path.join(save_sub_dir, f"{self.frame_count:06}.png")
                    chunk = reader.read(self.frame_size)
                    if len(chunk) < self.frame_size:
                        break

                    img = np.frombuffer(chunk, dtype=np.uint8).reshape(
                        self.height, self.width
                    )
                    cv2.imwrite(save_path, img)

                    self.frame_count += 1

    def convert_to_mp4(self, save_dir):
        if self.file_ext == ".bil.zst":
            for file in self.files:
                self.zst_convert_to_mp4(file, save_dir)
        elif self.file_ext == ".bil":
            for file in self.files:
                self.bil_convert_to_mp4(file, save_dir)
        else:
            raise ValueError(f"Invalid file ext: {self.file_ext}")

    def convert_to_avi(self, save_dir):
        if self.file_ext == ".bil.zst":
            for file in self.files:
                self.zst_convert_to_avi(file, save_dir)
        elif self.file_ext == ".bil":
            for file in self.files:
                self.bil_convert_to_avi(file, save_dir)
        else:
            raise ValueError(f"Invalid file ext: {self.file_ext}")

    def convert_to_png(self, save_dir):
        if self.file_ext == ".bil.zst":
            for file in self.files:
                self.zst_convert_to_png(file, save_dir)
        else:
            raise ValueError(f"Invalid file ext: {self.file_ext}")

    def convert(self, save_dir="./"):
        if self.output_ext == ".mp4":
            self.convert_to_mp4(save_dir)
        elif self.output_ext == ".avi":
            self.convert_to_avi(save_dir)
        elif self.output_ext == ".png":
            self.convert_to_png(save_dir)
        else:
            raise ValueError(f"Invalid output ext: {self.output_ext}")


if __name__ == "__main__":
    reader = BilReader(
        path="data/imgs/bil_zst/2025-04-20_10-21-07.bil.zst",
        file_ext=".bil.zst",
        output_ext=".png",
        is_correct=False,
    )
    reader.convert("data/imgs/imgs")
    print("Finished!")
