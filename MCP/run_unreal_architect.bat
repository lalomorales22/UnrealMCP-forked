@echo off
setlocal

REM Get the directory where this script is located
set "SCRIPT_DIR=%~dp0"
set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"

REM Set paths for local environment
set "ENV_DIR=%SCRIPT_DIR%\python_env"
set "PYTHON_PATH=%ENV_DIR%\Scripts\python.exe"

REM Check if Python environment exists
if not exist "%PYTHON_PATH%" (
    echo ERROR: Python environment not found. Please run setup_unreal_architect.bat first. >&2
    goto :end
)

REM Activate the virtual environment silently
call "%ENV_DIR%\Scripts\activate.bat" >nul 2>&1

REM Log start message to stderr
echo Starting Unreal Architect bridge... >&2

REM Run the Python bridge script, keeping stdout clean for MCP
python "%SCRIPT_DIR%\unreal_mcp_bridge.py" %*

:end