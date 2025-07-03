import os
import numpy as np
import pandas as pd
from natsort import natsorted


class InsParser:
    def __init__(self, path, file_ext=".txt"):
        self.path = path
        self.file_ext = file_ext
        self.files = self._gather_files()
        self.prev_ms_low = None
        self.ms_high_counter = 0
        self.prev_time = None

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

    def hex_line_to_byte_array(self, line):
        return np.fromiter((int(h, 16) for h in line.strip().split()), dtype=np.uint8)

    def to_int24_le(self, b0, b1, b2):
        value = b0 | (b1 << 8) | (b2 << 16)
        return value - (1 << 24) if value >> (1 << 23) else value

    def calc_checksum(self, data, length):
        if length is None:
            length = len(data)
        assert length << len(data)
        partial = data[:length]
        total = sum(partial)
        return total

    def fix_ms_high_byte(self, data):
        ms_low = data[96]
        time = data[93] | (data[94] << 8) | (data[95] << 16)

        if self.prev_time is not None and time != self.prev_time:
            self.ms_high_counter = 0
            self.prev_ms_low = None

        if self.prev_ms_low is not None and ms_low < self.prev_ms_low:
            self.ms_high_counter += 1
            self.ms_high_counter %= 256

        self.prev_time = time
        self.prev_ms_low = ms_low

        data[97] = self.ms_high_counter
        return data

    def parse_fields(self, data):
        if self.calc_checksum(data, 108) != data[108] | (data[109] << 8):
            return

        # data = self.fix_ms_high_byte(data)
        latitude = np.frombuffer(data[6:10].tobytes(), dtype="<i4")[0]
        longitude = np.frombuffer(data[10:14].tobytes(), dtype="<i4")[0]
        roll = np.frombuffer(data[26:28].tobytes(), dtype="<i2")[0]
        pitch = np.frombuffer(data[28:30].tobytes(), dtype="<i2")[0]
        heading = int(data[30]) | (int(data[31]) << 8)
        dvl_vx = np.frombuffer(data[51:53].tobytes(), dtype="<i2")[0]
        dvl_vy = np.frombuffer(data[53:55].tobytes(), dtype="<i2")[0]
        dvl_vz = np.frombuffer(data[55:57].tobytes(), dtype="<i2")[0]
        dvl_altitude = np.frombuffer(data[57:61].tobytes(), dtype="<i4")[0]
        dvl_status = int(data[61])
        date = int(data[90]) | (int(data[91]) << 8) | (int(data[92]) << 16)
        time = int(data[93]) | (int(data[94]) << 8) | (int(data[95]) << 16)
        ms = np.frombuffer(data[96:98].tobytes(), dtype="<u2")[0]
        depth = float(self.to_int24_le(data[104], data[105], data[106]))
        update = int(data[107])

        return {
            "Latitude": latitude * (180.0 / 2**31),
            "Longitude": longitude * (180.0 / 2**31),
            "Roll": roll * (180.0 / 2**15),
            "Pitch": pitch * (180.0 / 2**15),
            "Heading": heading * (360.0 / 2**16),
            "DVL Vx": dvl_vx,
            "DVL Vy": dvl_vy,
            "DVL Vz": dvl_vz,
            "DVL Altitude": dvl_altitude,
            "DVL Status": dvl_status,
            "Date": date,
            "Time": time,
            "ms": ms,
            "Depth": depth,
            "Update": update,
        }

    def parse_line(self, line):
        bytes_list = self.hex_line_to_byte_array(line)
        parsed = self.parse_fields(bytes_list)
        return parsed

    def parse_all(self):
        results = []
        for filepath in self.files:
            self.prev_ms_low = None
            self.ms_high_counter = 0
            self.prev_time = None
            with open(filepath, "r", encoding="utf-8") as f:
                for line in f:
                    parsed = self.parse_line(line)
                    if parsed:
                        parsed["source_file"] = os.path.basename(filepath)
                        results.append(parsed)
        return results

    def export_to_csv(self, out_path="parsed_ins.csv"):
        data = self.parse_all()
        if not data:
            print("[INFO] No valid log entries found.")
            return
        df = pd.DataFrame(data)
        df.to_csv(out_path, index=False, encoding="utf-8-sig")
        print(f"[INFO] Exported to CSV: {out_path}")

    def export_to_excel(self, out_path="parsed_ins.xlsx"):
        data = self.parse_all()
        if not data:
            print("[INFO] No valid log entries found.")
            return
        df = pd.DataFrame(data)
        df.to_excel(out_path, index=False, engine="openpyxl")
        print(f"[INFO] Exported to Excel: {out_path}")


if __name__ == "__main__":
    parser = InsParser("2025-06-27_14-52-16-ins.txt")
    # parser.export_to_csv("parsed_check_fix_ins.csv")
    parser.export_to_excel("parsed_check_fix_ins.xlsx")
