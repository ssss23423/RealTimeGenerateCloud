import cv2
import zstandard as zstd
import numpy as np


class ImageSaver:
    def __init__(self, save_path, img_width, img_height, fps, is_color=False):
        self.save_path = save_path
        self.img_width = img_width
        self.img_height = img_height
        self.fps = fps
        self.is_color = is_color

        self.writer = None
        self.saved_as_video = False
        self.saved_as_zstd = False

        self.zstd_compressor = None
        self.zstd_writer = None

        self._create_writer()

    def _create_writer(self):
        if self.save_path.endswith(".mp4"):
            fourcc = cv2.VideoWriter_fourcc(*"mp4v")
            self.writer = cv2.VideoWriter(
                self.save_path,
                fourcc,
                self.fps,
                (self.img_width, self.img_height),
                isColor=self.is_color,
            )
            self.saved_as_video = True
        elif self.save_path.endswith(".avi"):
            fourcc = cv2.VideoWriter_fourcc(*"XVID")
            self.writer = cv2.VideoWriter(
                self.save_path,
                fourcc,
                self.fps,
                (self.img_width, self.img_height),
                isColor=self.is_color,
            )
            self.saved_as_video = True
        elif self.save_path.endswith(".png"):
            pass
        elif self.save_path.endswith(".bil.zst"):
            self.zstd_compressor = zstd.ZstdCompressor(level=3)
            self.zstd_writer = open(self.save_path, "wb")
            self.saved_as_zstd = True
        else:
            raise ValueError(f"Invalid ext: {self.save_path}")

    def write(self, img):
        if self.saved_as_video:
            self.writer.write(img)
        elif self.saved_as_zstd:
            if len(img.shape) == 3:
                img = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
            raw = img.astype(np.uint8).tobytes()
            compressed = self.zstd_compressor.compress(raw)
            self.zstd_writer.write(compressed)
        else:
            # Save as PNG
            cv2.imwrite(self.save_path, img)

    def close(self):
        if self.saved_as_video:
            self.writer.release()
        if self.saved_as_zstd:
            self.zstd_writer.close()
