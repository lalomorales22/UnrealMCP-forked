import os
import re
from pathlib import Path

plugin_dir = Path(__file__).resolve().parent.parent
const_path = plugin_dir / "Source" / "UnrealArchitect" / "Public" / "MCPConstants.h"
port = 13377
if const_path.exists():
    text = const_path.read_text()
    m = re.search(r"DEFAULT_PORT\s*=\s*(\d+)", text)
    if m:
        port = int(m.group(1))
print(port)

