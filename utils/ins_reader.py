import pandas as pd
import numpy as np
from datetime import datetime
import warnings


class InsReader:
    def __init__(self, path):
        print(f"[Ins Reader] loading...")
        self.df = self._load_data(path)

        self.timestamps = self._compose_timestamp_vectorized(self.df)

        self.roll = self.df["Roll"].to_numpy()
        self.pitch = self.df["Pitch"].to_numpy()
        self.yaw = self.df["Heading"].to_numpy()

        print(f"INS 时间范围: {self.timestamps[0]:.3f}s ~ {self.timestamps[-1]:.3f}s")

    def _load_data(self, path):
        if path.endswith(".xlsx"):
            return pd.read_excel(path)
        elif path.endswith(".csv"):
            return pd.read_csv(path)
        else:
            raise ValueError(f"Invalid path: {path}")

    def _compose_timestamp_vectorized(self, df):
        date_strs = df["Date"].astype(str).str.zfill(6)
        time_strs = df["Time"].astype(str).str.zfill(6)
        ms_values = df["ms"].astype(int)

        years = 2000 + date_strs.str[:2].astype(int)
        months = date_strs.str[2:4].astype(int)
        days = date_strs.str[4:6].astype(int)

        hours = time_strs.str[:2].astype(int)
        minutes = time_strs.str[2:4].astype(int)
        seconds = time_strs.str[4:6].astype(int)

        datetimes = pd.to_datetime(
            {
                "year": years,
                "month": months,
                "day": days,
                "hour": hours,
                "minute": minutes,
                "second": seconds,
            }
        )

        timestamps = datetimes.astype("int64") / 1e9 + ms_values / 1000.0
        return timestamps.to_numpy()

    def get_interpolated_pose(self, target_time):
        if target_time <= self.timestamps[0]:
            warnings.warn("Target time before INS range, using first record.")
            return {
                "roll": self.roll[0],
                "pitch": self.pitch[0],
                "yaw": self.yaw[0],
            }
        elif target_time >= self.timestamps[-1]:
            warnings.warn("Target time beyond INS range, using first record.")
            return {
                "roll": self.roll[-1],
                "pitch": self.pitch[-1],
                "yaw": self.yaw[-1],
            }

        idx = np.searchsorted(self.timestamps, target_time)
        t0, t1 = self.timestamps[idx - 1], self.timestamps[idx]
        alpha = (target_time - t0) / (t1 - t0)

        def interp(a, b):
            return (1 - alpha) * a + alpha * b

        return {
            "roll": interp(self.roll[idx - 1], self.roll[idx]),
            "pitch": interp(self.pitch[idx - 1], self.pitch[idx]),
            "yaw": interp(self.yaw[idx - 1], self.yaw[idx]),
        }
