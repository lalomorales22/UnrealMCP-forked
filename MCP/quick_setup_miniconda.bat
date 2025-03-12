@echo off
echo Quick Setup for Unreal MCP with Miniconda...

REM Use the same Python interpreter that Claude Desktop is using for Blender
set PYTHON_CMD=C:\Users\mikek\miniconda3\python.exe

if not exist "%PYTHON_CMD%" (
    echo Miniconda Python not found at default location: %PYTHON_CMD%
    echo Please enter the path to your Miniconda/Anaconda Python executable:
    set /p PYTHON_CMD=Python path: 
    
    if not exist "%PYTHON_CMD%" (
        echo Python executable not found at: %PYTHON_CMD%
        echo Setup failed.
        pause
        exit /b 1
    )
)

echo.
echo Using Python: %PYTHON_CMD%
echo.

echo Installing MCP package...
%PYTHON_CMD% -m pip install mcp>=0.1.0

if %ERRORLEVEL% NEQ 0 (
    echo Failed to install MCP package.
    pause
    exit /b 1
)

echo.
echo Creating wrapper script for Miniconda Python...

(
    echo @echo off
    echo REM This wrapper script uses the Miniconda Python interpreter
    echo.
    echo set PYTHON_CMD=%PYTHON_CMD%
    echo.
    echo if not exist "%%PYTHON_CMD%%" (
    echo     echo Python not found at: %%PYTHON_CMD%%
    echo     echo Falling back to system Python...
    echo     set PYTHON_CMD=python
    echo ^)
    echo.
    echo %%PYTHON_CMD%% "%%~dp0unreal_mcp_server.py" %%*
) > "%~dp0run_unreal_mcp_miniconda.bat"

echo.
echo Wrapper script created: run_unreal_mcp_miniconda.bat
echo.

echo Updating Claude Desktop configuration...

(
    echo {
    echo     "mcpServers": {
    echo         "blender": {
    echo             "command": "C:\\Users\\mikek\\miniconda3\\Scripts\\uvx.exe",
    echo             "args": [
    echo                 "blender-mcp"
    echo             ]
    echo         },
    echo         "unreal": {
    echo             "command": "%~dp0run_unreal_mcp_miniconda.bat",
    echo             "args": []
    echo         }
    echo     }
    echo }
) > "%~dp0claude_desktop_config_miniconda.json"

echo.
echo Configuration file created: claude_desktop_config_miniconda.json
echo.

set /p choice=Do you want to copy the configuration file to Claude Desktop now? (Y/N): 
if /i "%choice%"=="Y" (
    if not exist "C:\Users\%USERNAME%\AppData\Roaming\Claude\" (
        mkdir "C:\Users\%USERNAME%\AppData\Roaming\Claude\"
    )
    copy /Y "%~dp0claude_desktop_config_miniconda.json" "C:\Users\%USERNAME%\AppData\Roaming\Claude\claude_desktop_config.json"
    echo Configuration file copied to Claude Desktop.
) else (
    echo Skipping configuration file copy.
)

echo.
echo Setup complete!
echo.
echo To test the MCP server directly, run: run_unreal_mcp_miniconda.bat
echo.
pause 