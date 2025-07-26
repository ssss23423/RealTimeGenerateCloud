import os
import logging
import time


def setup_logger(log_file, log_level=logging.INFO, is_record=True, format_set=True):
    logger = logging.getLogger(log_file)
    logger.setLevel(log_level)

    if is_record is True:
        file_handle = logging.FileHandler(log_file)
        file_handle.setLevel(log_level)
        if format_set is True:
            formatter = logging.Formatter("%(asctime)s - %(levelname)s: %(message)s")
            file_handle.setFormatter(formatter)
        logger.addHandler(file_handle)

    console_handler = logging.StreamHandler()
    console_handler.setLevel(log_level)
    if format_set is True:
        formatter = logging.Formatter("%(asctime)s - %(levelname)s: %(message)s")
        console_handler.setFormatter(formatter)
    logger.addHandler(console_handler)
    return logger


log_dir = "/data/project/GenerateCloud/logs"
local_time = time.strftime("%Y%m%d-%H%M%S")
if os.path.exists(log_dir) is False:
    os.makedirs(log_dir)
logger = setup_logger(os.path.join(log_dir, local_time + ".log"))
