import os
import re
import pandas as pd
from natsort import natsorted


class LogParser:
    def __init__(self, path, file_ext=".log"):
        self.path = path
        self.file_ext = file_ext
        self.line_pattern = re.compile(
            r"Local Time:\s*(?P<local_time>[^\s,]+),\s*"
            r"Time:\s*(?P<time>[^\s,]+),\s*"
            r"Depth:\s*(?P<depth>[\d.]+),\s*"
            r"Height:\s*(?P<height>[\d.]+),\s*"
            r"grab image:\s*(?P<grab_image>\d+)"
        )
        self.files = self._gather_files()

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

    def parse_line(self, line):
        match = self.line_pattern.search(line)
        return match.groupdict() if match else None

    def parse_all(self):
        results = []
        for filepath in self.files:
            with open(filepath, "r", encoding="utf-8") as f:
                for line in f:
                    parsed = self.parse_line(line)
                    if parsed:
                        parsed["source_file"] = os.path.basename(filepath)
                        results.append(parsed)
        return results

    def export_to_csv(self, out_path="parsed_log.csv"):
        data = self.parse_all()
        if not data:
            print("[INFO] No valid log entries found.")
            return
        df = pd.DataFrame(data)
        df.to_csv(out_path, index=False, encoding="utf-8-sig")
        print(f"[INFO] Exported to CSV: {out_path}")

    def export_to_excel(self, out_path="parsed_log.xlsx"):
        data = self.parse_all()
        if not data:
            print("[INFO] No valid log entries found.")
            return
        df = pd.DataFrame(data)
        df.to_excel(out_path, index=False, engine="openpyxl")
        print(f"[INFO] Exported to Excel: {out_path}")


if __name__ == "__main__":
    parser = LogParser("data/logs/raw/global_log.log")
    parser.export_to_csv("parsed_log.csv")
    parser.export_to_excel("parsed_log.xlsx")
