import cv2
import numpy as np


class ImageStabilizer:

    def __init__(self, cx, cy, sx, sy, focus):
        """
        :param cx: pixel
        :param cy: pixel
        :param sx: m/pixel
        :param sy: m/pixel
        :param focus: m
        """
        self.cx = cx
        self.cy = cy
        self.sx = sx
        self.sy = sy
        self.focus = focus

        self.K = self._calculate_intrinsics_from_halcon()

    def _calculate_intrinsics_from_halcon(self):
        self.fx = self.focus / self.sx
        self.fy = self.focus / self.sy

        K = np.array([[self.fx, 0, self.cx], [0, self.fy, self.cy], [0, 0, 1]])
        return K

    def euler_to_rotation_matrix(self, roll, pitch, yaw, degrees=True):
        if degrees is True:
            roll, pitch, yaw = np.deg2rad([roll, pitch, yaw])

        R_x = np.array(
            [
                [1, 0, 0],
                [0, np.cos(roll), -np.sin(roll)],
                [0, np.sin(roll), np.cos(roll)],
            ]
        )

        R_y = np.array(
            [
                [np.cos(pitch), 0, np.sin(pitch)],
                [0, 1, 0],
                [-np.sin(pitch), 0, np.cos(pitch)],
            ]
        )

        R_z = np.array(
            [[np.cos(yaw), -np.sin(yaw), 0], [np.sin(yaw), np.cos(yaw), 0], [0, 0, 1]]
        )

        return R_z @ R_y @ R_x

    def correct_image(self, img, roll, pitch, yaw, degrees=True):
        h, w = img.shape[:2]

        R_ned_to_body = self.euler_to_rotation_matrix(roll, pitch, yaw, degrees)
        R_body_to_cam = self.euler_to_rotation_matrix(0, 0, 180, degrees)

        R_ned_to_cam = R_body_to_cam @ R_ned_to_body
        # R_ned_to_cam = R_ned_to_body
        H = self.K @ R_ned_to_cam @ np.linalg.inv(self.K)
        corrected_img = cv2.warpPerspective(img, H, (w, h))
        return corrected_img


if __name__ == "__main__":
    stabilizer = ImageStabilizer(
        cx=664.923, cy=411.884, sx=4.68544e-06, sy=4.7e-06, focus=0.0111401
    )
    image = cv2.imread("data/imgs/imgs/2025-04-20_12-41-08/1.png")

    # roll = -5.5810546875
    # pitch = -5.16357421875
    # yaw = 314.5166015625
    # yaw = 0

    image = cv2.imread("data/imgs/imgs/2025-04-20_10-21-07/1.png")

    roll = -5.82275390625
    pitch = -2.2796630859375
    # yaw = 177
    yaw = 0

    corrected = stabilizer.correct_image(image, roll, pitch, yaw)

    cv2.imwrite("test1.png", corrected)
