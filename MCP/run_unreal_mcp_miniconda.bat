@echo off
REM This wrapper script uses the Miniconda Python interpreter

set PYTHON_CMD=C:\Users\mikek\miniconda3\python.exe

if not exist "%PYTHON_CMD%" (
    echo Python not found at: %PYTHON_CMD%
    echo Falling back to system Python...
    set PYTHON_CMD=python
)

%PYTHON_CMD% "%~dp0unreal_mcp_server.py" %*
