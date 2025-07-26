import pandas as pd
from datetime import datetime

from utils.log import logger


class LogTimeMapper:

    def __init__(self, log_path, fps=120):
        logger.info(f"[Log Time Mapper] loading...")
        self.fps = fps
        self.df = pd.read_csv(log_path)
        self.df = self.df.sort_values("grab_image").reset_index(drop=True)
        self.df["timestamp"] = self._parse_time(self.df["time"])

    def _parse_time(self, log_time_str):
        log_time_str = log_time_str.astype(str)

        years = 2000 + log_time_str.str.slice(0, 2).astype(int)
        months = log_time_str.str.slice(2, 4).astype(int)
        days = log_time_str.str.slice(4, 6).astype(int)

        hours = log_time_str.str.slice(7, 9).astype(int)
        minutes = log_time_str.str.slice(9, 11).astype(int)
        seconds = log_time_str.str.slice(11, 13).astype(int)

        datetimes = pd.to_datetime(
            {
                "year": years,
                "month": months,
                "day": days,
                "hour": hours,
                "minute": minutes,
                "second": seconds,
            },
            errors="coerce",
        )

        timestamps = datetimes.astype("int64") / 1e9
        return timestamps.to_numpy()

    def get_estimated_timestamp(self, image_idx, return_bounds=False):
        df = self.df
        idx_start = df.iloc[0]["grab_image"]
        idx_end = df.iloc[-1]["grab_image"]
        t_start = df.iloc[0]["timestamp"]
        t_end = df.iloc[-1]["timestamp"]

        if image_idx < idx_start:
            delta_frames = idx_start - image_idx
            est_time = t_start - delta_frames / self.fps
            if return_bounds:
                return est_time, (idx_start, t_start), (idx_start, t_start)
            return est_time
        elif image_idx > idx_end:
            delta_frames = image_idx - idx_end
            est_time = t_end + delta_frames / self.fps
            if return_bounds:
                return est_time, (idx_end, t_end), (idx_end, t_end)
            return est_time
        else:
            for i in range(len(df) - 1):
                g0 = df.loc[i, "grab_image"]
                g1 = df.loc[i + 1, "grab_image"]
                if g0 <= image_idx < g1:
                    t0 = df.loc[i, "timestamp"]
                    t1 = df.loc[i + 1, "timestamp"]
                    alpha = (image_idx - g0) / (g1 - g0)
                    est_time = t0 + alpha * (t1 - t0)
                    if return_bounds:
                        return est_time, (g0, t0), (g1, t1)
                    return est_time

            if return_bounds:
                return t_end, (idx_end, t_end), (idx_end, t_end)
            return t_end
