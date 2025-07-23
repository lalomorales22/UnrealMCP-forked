# MCP Server Setup Guide

This guide outlines what is needed to run the Unreal Architect MCP server and connect to it from Unreal Engine.

## Requirements
- Unreal Engine 5.5 or later
- Python 3.7+
- An MCP client (such as Claude Desktop)

## Installation Steps
1. Clone this repository into your Unreal project under `Plugins/UnrealArchitect`.
2. Regenerate project files and compile your project.
3. Enable **Unreal Architect** and the **Python Editor Script Plugin** within Unreal.
4. Navigate to `Plugins/UnrealArchitect/MCP/` and run `setup_unreal_architect.bat`.
   - This installs the `mcp` Python package and creates `run_unreal_architect.bat`.
5. (Optional) Configure your MCP client to launch `run_unreal_architect.bat` automatically.

## Using the Server
1. Start Unreal Engine with the plugin enabled.
2. Run `run_unreal_architect.bat` to start the MCP server.
3. Connect to `localhost:13377` using your MCP client.
4. Send commands such as `create_object`, `delete_object`, or `execute_python` to control the editor.

## Troubleshooting
- If the MCP client cannot connect, confirm the server is running and the port matches your settings.
- Re-run the setup script if the `mcp` Python package is missing.
- Claude Desktop logs can be found under `%APPDATA%\Claude\logs\` on Windows.
