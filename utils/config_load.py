import json


def load_config_json(config_file="config.json"):
    with open(config_file, "r") as file:
        config = json.load(file)

    return config
