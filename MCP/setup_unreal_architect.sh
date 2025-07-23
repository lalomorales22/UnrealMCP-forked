#!/usr/bin/env bash
set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ENV_DIR="$SCRIPT_DIR/python_env"
MODULES_DIR="$SCRIPT_DIR/python_modules"

if [ ! -d "$ENV_DIR" ]; then
    python3 -m venv "$ENV_DIR"
fi
source "$ENV_DIR/bin/activate"

pip install --upgrade pip
pip install -r "$SCRIPT_DIR/requirements.txt"

cat > "$SCRIPT_DIR/run_unreal_architect.sh" <<'RUNEOF'
#!/usr/bin/env bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/python_env/bin/activate"
PORT=$(python "$SCRIPT_DIR/get_port.py")
export MCP_PORT="$PORT"
python "$SCRIPT_DIR/unreal_mcp_bridge.py" --port "$PORT" "$@"
RUNEOF
chmod +x "$SCRIPT_DIR/run_unreal_architect.sh"

echo "Setup complete. Use run_unreal_architect.sh to start the bridge."

