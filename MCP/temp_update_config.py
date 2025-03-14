import json
import os
import sys

config_file = sys.argv[1]
run_script = sys.argv[2]

config = {}
try:
    with open(config_file, 'r') as f:
        config = json.load(f)
except (FileNotFoundError, json.JSONDecodeError):
    pass

config.setdefault('mcpServers', {})['unreal'] = {'command': run_script, 'args': []}

with open(config_file, 'w') as f:
    json.dump(config, f, indent=4)