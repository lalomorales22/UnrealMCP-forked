#!/usr/bin/env bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/python_env/bin/activate"
PORT=$(python "$SCRIPT_DIR/get_port.py")
export MCP_PORT="$PORT"
python "$SCRIPT_DIR/unreal_mcp_bridge.py" --port "$PORT" "$@"

