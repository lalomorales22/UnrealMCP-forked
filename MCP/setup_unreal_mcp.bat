@echo off
echo Setting up Unreal MCP Server...

REM Check if Python is installed
python --version >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo Python is not installed or not in PATH.
    echo Please install Python from https://www.python.org/downloads/
    pause
    exit /b 1
)

echo.
echo Which Python environment should be used?
echo 1. System Python (default)
echo 2. Miniconda/Anaconda Python
echo 3. Custom Python path
echo.
set /p python_choice=Enter your choice (1-3): 

set PYTHON_CMD=python
if "%python_choice%"=="2" (
    set PYTHON_CMD=C:\Users\%USERNAME%\miniconda3\python.exe
    if not exist "%PYTHON_CMD%" (
        echo Miniconda Python not found at default location.
        set /p conda_path=Enter the path to your conda Python executable: 
        set PYTHON_CMD=%conda_path%
    )
) else if "%python_choice%"=="3" (
    set /p custom_python=Enter the full path to your Python executable: 
    set PYTHON_CMD=%custom_python%
)

echo.
echo Using Python: %PYTHON_CMD%
echo.

echo Installing required packages...
%PYTHON_CMD% -m pip install mcp>=0.1.0

if %ERRORLEVEL% NEQ 0 (
    echo Failed to install required packages.
    pause
    exit /b 1
)

echo.
echo Creating a wrapper script to ensure the correct Python is used...

(
    echo @echo off
    echo "%PYTHON_CMD%" "%%~dp0unreal_mcp_server.py" %%*
) > "%~dp0run_unreal_mcp.bat"

echo.
echo Setup complete!
echo.
echo To use the Unreal MCP server with Claude Desktop:
echo 1. Make sure your Unreal Engine with MCP plugin is running on port 9876
echo 2. Update the claude_desktop_config.json file to use the wrapper script:
echo.

(
    echo {
    echo     "mcpServers": {
    echo         "unreal": {
    echo             "command": "%~dp0run_unreal_mcp.bat",
    echo             "args": []
    echo         }
    echo     }
    echo }
) > "%~dp0claude_desktop_config_new.json"

echo New configuration file created: claude_desktop_config_new.json
echo.

set /p choice=Do you want to copy the new config file to Claude Desktop now? (Y/N): 
if /i "%choice%"=="Y" (
    if not exist "C:\Users\%USERNAME%\AppData\Roaming\Claude\" (
        mkdir "C:\Users\%USERNAME%\AppData\Roaming\Claude\"
    )
    copy /Y "%~dp0claude_desktop_config_new.json" "C:\Users\%USERNAME%\AppData\Roaming\Claude\claude_desktop_config.json"
    echo Config file copied successfully!
) else (
    echo Skipping config file copy.
)

echo.
echo To test the MCP server directly, run: run_unreal_mcp.bat
echo.
echo Setup completed. See README_MCP_SETUP.md for more details.
pause 