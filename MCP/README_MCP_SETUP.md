# Unreal Engine MCP Interface Setup

This guide explains how to set up the Model Context Protocol (MCP) interface for Unreal Engine to work with Claude Desktop.

## Prerequisites

1. Python installed on your system (3.7 or newer)
2. Claude Desktop application
3. Unreal Engine with the UnrealMCP plugin enabled

## Quick Setup

The setup process has been simplified to use a single setup script that handles all installation scenarios:

```
Plugins\UnrealMCP\MCP\setup_unreal_mcp.bat
```

This script will:

1. Detect available Python environments on your system (System Python, Miniconda/Anaconda, Claude Desktop environment)
2. Let you choose which Python environment to use
3. Install the required `mcp` package in the selected environment
4. Create a single run script (`run_unreal_mcp.bat`) that uses the correct Python environment
5. Create or update the Claude Desktop configuration file

### Python Environment Options

The setup script supports several Python environment options:

1. **System Python**: Uses the Python installation available in your system PATH
2. **Miniconda/Anaconda**: Uses a Python environment from Miniconda/Anaconda (recommended if you use Claude Desktop with Blender)
3. **Claude Desktop Environment**: Uses the Python environment that comes with Claude Desktop (if available)
4. **Custom Python Path**: Allows you to specify a custom Python executable path

## Manual Configuration

If you need to manually configure the MCP interface:

### 1. Install Required Python Package

```
python -m pip install mcp>=0.1.0
```

### 2. Create a Run Script

Create a batch file (`run_unreal_mcp.bat`) with the following content:

```batch
@echo off
setlocal
cd /d "%~dp0"
python "%~dp0unreal_mcp_server.py"
```

### 3. Configure Claude Desktop

Create or update the Claude Desktop configuration file at:
```
%APPDATA%\Claude\claude_desktop_config.json
```

With the following content:
```json
{
    "mcpServers": {
        "unreal": {
            "command": "C:\\Path\\To\\Your\\Plugins\\UnrealMCP\\MCP\\run_unreal_mcp.bat",
            "args": []
        }
    }
}
```

Replace the path with the actual path to your `run_unreal_mcp.bat` file.

## Troubleshooting

### Common Issues

1. **"No module named 'mcp'"**
   - The `mcp` package is not installed in the Python environment that Claude Desktop is using
   - Solution: Run the setup script again and make sure to select the correct Python environment

2. **Connection refused errors**
   - The Unreal Engine MCP server is not running or not listening on port 1337
   - Solution: Make sure your Unreal Engine is running with the MCP plugin enabled
   - Check that the port setting in the MCP plugin matches the default (1337)

3. **Claude Desktop can't start the MCP server**
   - Check the logs at: `%APPDATA%\Claude\logs\mcp-server-unreal.log`
   - Make sure the path in the configuration file is correct
   - Verify that the wrapper batch file exists and points to the correct Python interpreter

### Checking Logs

Claude Desktop logs MCP server output to:
```
%APPDATA%\Claude\logs\mcp-server-unreal.log
```

Check this file for error messages if you're having trouble.

## Usage

1. Start your Unreal Engine project with the MCP plugin enabled
2. Launch Claude Desktop
3. In Claude Desktop, you can now use commands like:
   - "Show me what's in the current Unreal scene"
   - "Create a cube at position [0, 0, 100]"
   - "Modify the object named 'Cube_1' to have scale [2, 2, 2]"
   - "Delete the object named 'Cube_1'"

## Available Commands

The MCP interface supports the following commands:

- `get_scene_info`: Retrieve information about the current scene
- `create_object`: Spawn a new object in the scene
- `modify_object`: Change properties of an existing object
- `delete_object`: Remove an object from the scene

## Testing the MCP Server Directly

You can test the MCP server directly without Claude Desktop by running:

```
Plugins\UnrealMCP\MCP\run_unreal_mcp.bat
```

This will start the MCP server using the correct Python interpreter and listen for connections from Claude Desktop. 