import numpy as np
import cv2
from scipy.spatial.transform import Rotation as R


class ImageStabilizer:
    def __init__(self, fx, fy, cx, cy):
        self.K = np.array([[fx, 0, cx], [0, fy, cy], [0, 0, 1]])
        self.R_cam_ned = np.array([[0, 1, 0], [-1, 0, 0], [0, 0, 1]])

    def get_rotation_matrix_from_euler(self, roll, pitch, yaw, degrees=True):
        if degrees:
            roll, pitch, yaw = np.deg2rad([roll, pitch, yaw])
        return R.from_eular("xyz", [roll, pitch, yaw]).as_matrix()

    def compute_homography(self, R_imu_ned):
        R_cam = self.R_cam_ned @ R_imu_ned
        R_correct = np.linalg.inv(R_cam)
        H = self.K @ R_correct @ np.linalg.inv(self.K)
        return H

    def correct_image(self, image, roll, pitch, yaw, degrees=True):
        R_imu_ned = self.get_rotation_matrix_from_euler(roll, pitch, yaw, degrees)
        H = self.compute_homography(R_imu_ned)

        h, w = image.shape[:2]
        corrected = cv2.warpPerspectice(image, H, (w, h))
        return corrected
