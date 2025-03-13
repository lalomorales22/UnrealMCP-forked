@echo off
setlocal enabledelayedexpansion

echo ========================================================
echo Unreal MCP - Python Environment Setup
echo ========================================================
echo.

REM Get the directory where this script is located
set "SCRIPT_DIR=%~dp0"
set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"

REM Set paths for local environment
set "ENV_DIR=%SCRIPT_DIR%\python_env"
set "MODULES_DIR=%SCRIPT_DIR%\python_modules"

echo Setting up Python environment in: %ENV_DIR%
echo.

REM Check if Python is installed
where python >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo ERROR: Python is not installed or not in your PATH.
    echo Please install Python and try again.
    goto :end
)

REM Get Python version and path
for /f "tokens=*" %%i in ('python --version 2^>^&1') do set PYTHON_VERSION=%%i
for /f "tokens=*" %%i in ('where python') do set SYSTEM_PYTHON=%%i
echo Detected %PYTHON_VERSION% at %SYSTEM_PYTHON%
echo.

REM Create directories if they don't exist
if not exist "%ENV_DIR%" (
    echo Creating Python environment directory...
    mkdir "%ENV_DIR%"
)

if not exist "%MODULES_DIR%" (
    echo Creating Python modules directory...
    mkdir "%MODULES_DIR%"
)

REM Check if virtualenv is installed
python -c "import virtualenv" >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo Installing virtualenv...
    python -m pip install virtualenv
)

REM Create virtual environment if it doesn't exist
if not exist "%ENV_DIR%\Scripts\python.exe" (
    echo Creating virtual environment...
    python -m virtualenv "%ENV_DIR%"
) else (
    echo Virtual environment already exists.
)

REM Activate the virtual environment and install packages
echo.
echo Activating virtual environment and installing packages...
call "%ENV_DIR%\Scripts\activate.bat"

REM Check if activation was successful
if %ERRORLEVEL% neq 0 (
    echo ERROR: Failed to activate virtual environment.
    goto :end
)

REM Install MCP package in the virtual environment
echo Installing MCP package...
python -m pip install mcp>=0.1.0

REM Also install to modules directory as a backup
echo Installing MCP package to modules directory as backup...
python -m pip install mcp>=0.1.0 -t "%MODULES_DIR%"

REM Verify installation
echo.
echo Verifying MCP installation...
python -c "import mcp; print(f'MCP package installed successfully. Version: {getattr(mcp, \"__version__\", \"unknown\")}')"

REM Create a configuration file for Claude Desktop
set "CLAUDE_CONFIG_DIR=%APPDATA%\Claude"
set "CLAUDE_CONFIG_FILE=%CLAUDE_CONFIG_DIR%\claude_desktop_config.json"

REM Create Claude config directory if it doesn't exist
if not exist "%CLAUDE_CONFIG_DIR%" (
    mkdir "%CLAUDE_CONFIG_DIR%"
    echo Created Claude configuration directory.
)

echo.
echo Creating Claude Desktop configuration...
set "PYTHON_PATH=%ENV_DIR%\Scripts\python.exe"
set "PYTHON_PATH_JSON=%PYTHON_PATH:\=\\%"

echo {> "%CLAUDE_CONFIG_FILE%"
echo   "tools": {>> "%CLAUDE_CONFIG_FILE%"
echo     "unreal": {>> "%CLAUDE_CONFIG_FILE%"
echo       "pythonPath": "%PYTHON_PATH_JSON%",>> "%CLAUDE_CONFIG_FILE%"
echo       "connection": {>> "%CLAUDE_CONFIG_FILE%"
echo         "type": "local",>> "%CLAUDE_CONFIG_FILE%"
echo         "port": 1337>> "%CLAUDE_CONFIG_FILE%"
echo       }>> "%CLAUDE_CONFIG_FILE%"
echo     }>> "%CLAUDE_CONFIG_FILE%"
echo   }>> "%CLAUDE_CONFIG_FILE%"
echo }>> "%CLAUDE_CONFIG_FILE%"

echo Claude Desktop configuration created at: %CLAUDE_CONFIG_FILE%

REM Create the run script
echo Creating run script...
(
echo @echo off
echo setlocal
echo.
echo REM Get the directory where this script is located
echo set "SCRIPT_DIR=%%~dp0"
echo set "SCRIPT_DIR=%%SCRIPT_DIR:~0,-1%%"
echo.
echo REM Set paths for local environment
echo set "ENV_DIR=%%SCRIPT_DIR%%\python_env"
echo set "PYTHON_PATH=%%ENV_DIR%%\Scripts\python.exe"
echo.
echo REM Check if Python environment exists
echo if not exist "%%PYTHON_PATH%%" (
echo     echo ERROR: Python environment not found. Please run setup_unreal_mcp.bat first. ^>^&2
echo     goto :end
echo )
echo.
echo REM Activate the virtual environment silently
echo call "%%ENV_DIR%%\Scripts\activate.bat" ^>nul 2^>^&1
echo.
echo REM Log start message to stderr
echo echo Starting Unreal MCP bridge... ^>^&2
echo.
echo REM Run the Python bridge script
echo python "%%SCRIPT_DIR%%\unreal_mcp_bridge.py" %%*
echo.
echo :end
) > "%SCRIPT_DIR%\run_unreal_mcp.bat"

echo.
echo ========================================================
echo Setup complete!
echo.
echo To use with Claude Desktop:
echo 1. Run run_unreal_mcp.bat to start the MCP bridge
echo 2. Open Claude Desktop and it should automatically use the correct configuration
echo ========================================================
echo.
echo Press any key to exit...
pause > nul

:end
deactivate 