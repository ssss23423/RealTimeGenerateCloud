import os
import cv2
import numpy as np
from natsort import natsorted
import zstandard as zstd

from utils.log import logger


class BinaryReader:
    def __init__(self, path, img_width, img_height, file_ext):
        logger.info(f"[Binary Reader] loading...")
        self.path = path
        self.img_width = img_width
        self.img_height = img_height
        self.frame_size = self.img_width * self.img_height
        self.file_ext = file_ext
        self.files = self._gather_files()
        self.file_idx = 0
        self.current_file_f = None
        self.reader = None
        self.is_compressed = False
        self.zstd_dctx = zstd.ZstdDecompressor()
        self._open_next_file()

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

    def _open_next_file(self):
        if self.reader:
            self.reader.close()
        if self.current_file_f:
            self.current_file_f.close()

        if self.file_idx >= len(self.files):
            self.current_file_f = None
            self.reader = None
            self.current_file_name = None
            self.just_opened = False
            return

        file_path = self.files[self.file_idx]
        self.file_idx += 1

        self.current_file_name = os.path.basename(file_path)
        self.current_file_f = open(file_path, "rb")
        self.is_compressed = file_path.endswith(".bil.zst")

        if self.is_compressed:
            self.reader = self.zstd_dctx.stream_reader(self.current_file_f)
        else:
            self.reader = self.current_file_f

        self.just_opened = True

    def read_frame(self):
        if self.reader is None:
            return None, None

        chunk = self.reader.read(self.frame_size)
        if len(chunk) < self.frame_size:
            self._open_next_file()
            return self.read_frame()

        frame = np.frombuffer(chunk, dtype=np.uint8)
        first_frame_of_file = self.just_opened
        self.just_opened = False
        return frame.reshape((self.img_height, self.img_width)), (
            self.current_file_name if first_frame_of_file else None
        )

    def close(self):
        if self.reader:
            self.reader.close()
        if self.current_file_f:
            self.current_file_f.close()
