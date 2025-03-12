@echo off
echo Installing MCP package for Claude Desktop...

REM Use the same Python interpreter that Claude Desktop is using for Blender
set PYTHON_CMD=C:\Users\mikek\miniconda3\python.exe

if not exist "%PYTHON_CMD%" (
    echo Miniconda Python not found at default location: %PYTHON_CMD%
    echo Falling back to system Python...
    set PYTHON_CMD=python
)

echo Using Python: %PYTHON_CMD%
%PYTHON_CMD% "%~dp0install_mcp.py"
pause 