# Unreal Engine MCP Interface Setup

This guide explains how to set up the Model Context Protocol (MCP) interface for Unreal Engine to work with Claude Desktop.

## Prerequisites

1. Python installed on your system
2. Claude Desktop application
3. Unreal Engine with the MCP plugin running

## Quick Setup

### Option 1: Standard Setup
For a general setup that works with any Python environment:

```
Plugins\UnrealMCP\setup_unreal_mcp.bat
```

This script will:
1. Ask which Python environment to use (system, Miniconda, or custom)
2. Install the required `mcp` package in that environment
3. Create a wrapper script to ensure the correct Python is used
4. Generate a new Claude Desktop configuration file
5. Offer to copy the configuration file to the Claude Desktop location

### Option 2: Miniconda Setup (Recommended)
If Claude Desktop is using Miniconda for other MCP servers (like Blender), use this setup:

```
Plugins\UnrealMCP\quick_setup_miniconda.bat
```

This script will:
1. Automatically use the same Python environment that Claude Desktop uses for Blender
2. Install the required `mcp` package in that environment
3. Create a Miniconda-specific wrapper script
4. Generate a configuration file that matches your Blender setup
5. Offer to copy the configuration file to the Claude Desktop location

## Troubleshooting MCP Installation

If you're having issues with the MCP package not being found, you can use the dedicated installation script:

```
Plugins\UnrealMCP\install_mcp.bat
```

This will:
1. Show which Python interpreter is being used
2. Check if the MCP package is already installed
3. Install the MCP package if needed
4. Verify the installation was successful

**Important**: Make sure you run this script with the same Python interpreter that Claude Desktop will use.

## Manual Setup Instructions

### 1. Install Required Python Packages

```
python -m pip install mcp>=0.1.0
```

### 2. Test Connection to Unreal Engine

Before configuring Claude Desktop, verify that your Unreal Engine MCP server is running and accessible:

```
python Plugins\UnrealMCP\test_unreal_connection.py
```

This will test if your Unreal Engine C++ MCP server is running on port 9876 and responding to commands.

### 3. Configure Claude Desktop

Copy the generated configuration file to:
```
C:\Users\[YourUsername]\AppData\Roaming\Claude\claude_desktop_config.json
```

Or update your existing configuration file to include:
```json
{
    "mcpServers": {
        "unreal": {
            "command": "D:\\AISlop\\UnrealMCP_Dev\\Plugins\\UnrealMCP\\run_unreal_mcp_miniconda.bat",
            "args": []
        }
    }
}
```

Make sure the path points to the wrapper batch file created by the setup script.

### 4. Verify Your Unreal C++ MCP Server

Ensure your Unreal Engine C++ MCP server is:
- Running and listening on port 9876
- Accepting the commands defined in the Python script:
  - `get_scene_info`
  - `create_object`
  - `modify_object`
  - `delete_object`

## Usage

1. Start your Unreal Engine project with the MCP plugin enabled
2. Launch Claude Desktop
3. In Claude Desktop, you can now use commands like:
   - "Show me what's in the current Unreal scene"
   - "Create a cube at position [0, 0, 100]"
   - "Modify the object named 'Cube_1' to have scale [2, 2, 2]"
   - "Delete the object named 'Cube_1'"

## Troubleshooting

### Common Issues

1. **"No module named 'mcp'"**
   - The `mcp` package is not installed in the Python environment that Claude Desktop is using
   - Solution: Run `Plugins\UnrealMCP\quick_setup_miniconda.bat` to use the same Python environment as Blender
   - Alternatively, manually install with `python -m pip install mcp` in the correct environment

2. **"'run_unreal_mcp.bat' is not recognized as an internal or external command"**
   - The wrapper batch file doesn't exist or the path is incorrect
   - Solution: Run one of the setup scripts to create the wrapper file
   - Make sure the path in the Claude Desktop configuration matches the actual location of the file

3. **Connection refused errors**
   - The Unreal Engine C++ MCP server is not running or not listening on port 9876
   - Solution: Make sure your Unreal Engine is running with the MCP plugin enabled

4. **Claude Desktop can't start the MCP server**
   - Check the logs at: `C:\Users\[YourUsername]\AppData\Roaming\Claude\logs\mcp-server-unreal.log`
   - Make sure the path in the configuration file is correct
   - Verify that the wrapper batch file exists and points to the correct Python interpreter

5. **Invalid JSON responses**
   - The Unreal Engine C++ MCP server is returning malformed JSON
   - Check your C++ server implementation for formatting issues

### Checking Logs

Claude Desktop logs MCP server output to:
```
C:\Users\[YourUsername]\AppData\Roaming\Claude\logs\mcp-server-unreal.log
```

Check this file for error messages if you're having trouble.

## Extending Functionality

To add more tools to the MCP interface:

1. Add new methods to the Unreal Engine C++ MCP server
2. Add corresponding functions to the `unreal_mcp_server.py` script using the `@mcp.tool()` decorator
3. Restart Claude Desktop to pick up the new tools

## Advanced: Testing the MCP Server Directly

You can test the MCP server directly without Claude Desktop:

```
Plugins\UnrealMCP\run_unreal_mcp_miniconda.bat
```

This will start the MCP server using the correct Python interpreter and listen for connections from Claude Desktop. 