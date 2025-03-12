@echo off
REM This wrapper script ensures the correct Python interpreter is used for the MCP server
REM It will be updated by setup_unreal_mcp.bat when run

REM Default to system Python if not configured
python "%~dp0unreal_mcp_server.py" %* 