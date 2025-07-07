import cv2


class ImageSaver:
    def __init__(self, save_path, img_width, img_height, fps, is_color=False):
        self.save_path = save_path
        self.img_width = img_width
        self.img_height = img_height
        self.fps = fps
        self.is_color = is_color

        self.writer, self.saved_as_video = self._create_writer()

    def _create_writer(self):
        if self.save_path.endswith(".mp4"):
            fourcc = cv2.VideoWriter_fourcc(*"mp4v")
        elif self.save_path.endswith(".avi"):
            fourcc = cv2.VideoWriter_fourcc(*"XVID")
        elif self.save_path.endswith(".png"):
            return (None, False)
        else:
            raise ValueError(f"Invalid ext: {self.path}")

        return (
            cv2.VideoWriter(
                self.save_path,
                fourcc,
                self.fps,
                (self.img_width, self.img_height),
                isColor=self.is_color,
            ),
            True,
        )

    def write(self, img):
        if self.saved_as_video:
            self.writer.write(img)
        else:
            cv2.imwrite(self.save_path, img)

    def close(self):
        if self.saved_as_video:
            self.writer.release()
