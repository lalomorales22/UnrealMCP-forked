@echo off
setlocal enabledelayedexpansion

echo ========================================================
echo Unreal Engine MCP Setup
echo ========================================================
echo.

:: Get the directory where this script is located
set "SCRIPT_DIR=%~dp0"
set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"

:: Set paths using MCPConstants
set "PLUGIN_ROOT=%SCRIPT_DIR%\.."
set "MCP_SCRIPTS_DIR=%SCRIPT_DIR%"
set "CLAUDE_CONFIG_DIR=%APPDATA%\Claude"
set "CLAUDE_CONFIG_FILE=%CLAUDE_CONFIG_DIR%\claude_desktop_config.json"

:: Create Claude config directory if it doesn't exist
if not exist "%CLAUDE_CONFIG_DIR%" (
    mkdir "%CLAUDE_CONFIG_DIR%"
    echo Created Claude configuration directory.
)

:: Check if Python is installed
where python >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo ERROR: Python is not installed or not in your PATH.
    echo Please install Python and try again.
    goto :end
)

:: Detect Python environments
echo Detecting Python environments...
echo.

:: Check for system Python
set "SYSTEM_PYTHON="
for /f "tokens=*" %%i in ('where python') do (
    set "SYSTEM_PYTHON=%%i"
    goto :found_system_python
)
:found_system_python

:: Check for Miniconda/Anaconda
set "CONDA_PATH="
if exist "%LOCALAPPDATA%\miniconda3\Scripts\activate.bat" (
    set "CONDA_PATH=%LOCALAPPDATA%\miniconda3"
) else if exist "%USERPROFILE%\miniconda3\Scripts\activate.bat" (
    set "CONDA_PATH=%USERPROFILE%\miniconda3"
) else if exist "%LOCALAPPDATA%\Continuum\miniconda3\Scripts\activate.bat" (
    set "CONDA_PATH=%LOCALAPPDATA%\Continuum\miniconda3"
) else if exist "%LOCALAPPDATA%\Anaconda3\Scripts\activate.bat" (
    set "CONDA_PATH=%LOCALAPPDATA%\Anaconda3"
) else if exist "%USERPROFILE%\Anaconda3\Scripts\activate.bat" (
    set "CONDA_PATH=%USERPROFILE%\Anaconda3"
)

:: Check for Claude Desktop's Python environment
set "CLAUDE_ENV_PATH="
if exist "%APPDATA%\Claude\claude-env" (
    set "CLAUDE_ENV_PATH=%APPDATA%\Claude\claude-env"
)

:: Display detected environments
echo Available Python environments:
echo.
echo 1. System Python
if not "%SYSTEM_PYTHON%"=="" (
    echo    Path: %SYSTEM_PYTHON%
) else (
    echo    Not found
)
echo.

echo 2. Miniconda/Anaconda
if not "%CONDA_PATH%"=="" (
    echo    Path: %CONDA_PATH%
) else (
    echo    Not found
)
echo.

echo 3. Claude Desktop Environment
if not "%CLAUDE_ENV_PATH%"=="" (
    echo    Path: %CLAUDE_ENV_PATH%
) else (
    echo    Not found
)
echo.

:: Ask user which environment to use
echo Which Python environment would you like to use?
echo 1. System Python
echo 2. Miniconda/Anaconda (recommended if you use Claude Desktop with Blender)
echo 3. Claude Desktop Environment (if available)
echo 4. Custom Python path
set /p ENV_CHOICE="Enter choice (1-4): "

set "PYTHON_PATH="
set "PYTHON_ACTIVATE="
set "ENV_NAME="

if "%ENV_CHOICE%"=="1" (
    set "PYTHON_PATH=%SYSTEM_PYTHON%"
    set "ENV_TYPE=system"
) else if "%ENV_CHOICE%"=="2" (
    if "%CONDA_PATH%"=="" (
        echo Miniconda/Anaconda not found.
        goto :end
    )
    
    echo.
    echo Available Conda environments:
    call "%CONDA_PATH%\Scripts\conda.exe" env list
    echo.
    set /p ENV_NAME="Enter environment name (or press Enter for base): "
    if "%ENV_NAME%"=="" set "ENV_NAME=base"
    
    set "PYTHON_ACTIVATE=call "%CONDA_PATH%\Scripts\activate.bat" %ENV_NAME% &&"
    set "PYTHON_PATH=%CONDA_PATH%\python.exe"
    set "ENV_TYPE=conda"
) else if "%ENV_CHOICE%"=="3" (
    if "%CLAUDE_ENV_PATH%"=="" (
        echo Claude Desktop environment not found.
        goto :end
    )
    
    set "PYTHON_PATH=%CLAUDE_ENV_PATH%\python.exe"
    set "ENV_TYPE=claude"
) else if "%ENV_CHOICE%"=="4" (
    set /p PYTHON_PATH="Enter full path to Python executable: "
    if not exist "%PYTHON_PATH%" (
        echo The specified Python path does not exist.
        goto :end
    )
    set "ENV_TYPE=custom"
) else (
    echo Invalid choice.
    goto :end
)

echo.
echo Installing MCP package...
echo.

:: Install MCP package
if "%ENV_TYPE%"=="conda" (
    %PYTHON_ACTIVATE% python -m pip install mcp>=0.1.0
) else (
    "%PYTHON_PATH%" -m pip install mcp>=0.1.0
)

:: Verify installation
echo.
echo Verifying MCP installation...
echo.

if "%ENV_TYPE%"=="conda" (
    %PYTHON_ACTIVATE% python -c "import mcp; print(f'MCP version {mcp.__version__} installed successfully')"
) else (
    "%PYTHON_PATH%" -c "import mcp; print(f'MCP version {mcp.__version__} installed successfully')"
)

if %ERRORLEVEL% neq 0 (
    echo ERROR: Failed to install MCP package.
    goto :end
)

:: Create run script
echo.
echo Creating run script...
echo.

(
    echo @echo off
    echo setlocal
    echo.
    if "%ENV_TYPE%"=="conda" (
        echo call "%CONDA_PATH%\Scripts\activate.bat" %ENV_NAME%
    )
    echo cd /d "%SCRIPT_DIR%"
    if "%ENV_TYPE%"=="conda" (
        echo python "%SCRIPT_DIR%\unreal_mcp_server.py"
    ) else (
        echo "%PYTHON_PATH%" "%SCRIPT_DIR%\unreal_mcp_server.py"
    )
) > "%SCRIPT_DIR%\run_unreal_mcp.bat"

echo Created run script: %SCRIPT_DIR%\run_unreal_mcp.bat

:: Create Claude Desktop configuration
echo.
echo Creating Claude Desktop configuration...
echo.

:: Check if config file already exists
set "CONFIG_EXISTS="
if exist "%CLAUDE_CONFIG_FILE%" set "CONFIG_EXISTS=1"

:: Create or update config
if "%CONFIG_EXISTS%"=="1" (
    echo Claude Desktop configuration already exists.
    set /p UPDATE_CONFIG="Would you like to update it? (y/n): "
    if /i not "%UPDATE_CONFIG%"=="y" goto :skip_config
    
    :: Backup existing config
    copy "%CLAUDE_CONFIG_FILE%" "%CLAUDE_CONFIG_FILE%.bak"
    echo Backed up existing configuration to %CLAUDE_CONFIG_FILE%.bak
) else (
    :: Create new config
    (
        echo {
        echo     "mcpServers": {
        echo         "unreal": {
        echo             "command": "%SCRIPT_DIR:\=\\%\\run_unreal_mcp.bat",
        echo             "args": []
        echo         }
        echo     }
        echo }
    ) > "%CLAUDE_CONFIG_FILE%"
    
    echo Created new Claude Desktop configuration.
    goto :config_done
)

:: Update existing config
powershell -Command "$config = Get-Content -Raw '%CLAUDE_CONFIG_FILE%' | ConvertFrom-Json; if (-not $config.mcpServers) { $config | Add-Member -MemberType NoteProperty -Name 'mcpServers' -Value @{} }; $config.mcpServers.unreal = @{ command='%SCRIPT_DIR:\=\\%\\run_unreal_mcp.bat'; args=@() }; $config | ConvertTo-Json -Depth 10 | Set-Content '%CLAUDE_CONFIG_FILE%'"

echo Updated Claude Desktop configuration.

:config_done
:skip_config

echo.
echo ========================================================
echo Setup Complete!
echo ========================================================
echo.
echo To use the Unreal MCP with Claude Desktop:
echo.
echo 1. Start your Unreal Engine project with the MCP plugin enabled
echo 2. Launch Claude Desktop
echo 3. Claude should now be able to control Unreal Engine
echo.
echo If you encounter any issues, check the logs at:
echo %APPDATA%\Claude\logs\mcp-server-unreal.log
echo.

:end
pause 