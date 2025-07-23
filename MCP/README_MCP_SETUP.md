Here's the README in Markdown format (already used in the previous response, but I'll ensure it's clean and ready for copy-pasting). You can directly copy this into a `README.md` file for your GitHub repository:

```markdown
# Unreal Engine MCP Interface

This project provides a Model Context Protocol (MCP) interface for Unreal Engine, enabling seamless integration with Claude Desktop. With this interface, users can interact with Unreal Engine using natural language commands through Claude Desktop, simplifying scene management and object manipulation.

## Table of Contents

- [Prerequisites](#prerequisites)
- [Quick Setup](#quick-setup)
- [Manual Configuration](#manual-configuration)
- [Troubleshooting](#troubleshooting)
- [Usage](#usage)
- [Available Commands](#available-commands)
- [Testing the MCP Server Directly](#testing-the-mcp-server-directly)

## Prerequisites

To set up the MCP interface, ensure you have the following:

- **Python 3.7 or newer** installed on your system
- **Claude Desktop** application
- **Unreal Engine** with the UnrealArchitect plugin enabled

## Quick Setup

The setup process is streamlined with a single script that handles all installation scenarios:

1. Navigate to the `Plugins\UnrealArchitect\MCP\` directory.
2. Run the following script:
   ```
   Plugins\UnrealArchitect\MCP\setup_unreal_architect.bat
   ```

This script will:

- Detect available Python environments (System Python, Miniconda/Anaconda, Claude Desktop environment)
- Prompt you to choose a Python environment
- Install the required `mcp` package in the selected environment
- Generate a `run_unreal_architect.bat` script tailored to the chosen Python environment
- Create or update the Claude Desktop configuration file

### Python Environment Options

The setup script supports multiple Python environment options:

1. **System Python**: Uses the Python installation in your system PATH.
2. **Miniconda/Anaconda**: Uses a Python environment from Miniconda/Anaconda (recommended for users integrating with Blender via Claude Desktop).
3. **Claude Desktop Environment**: Uses the Python environment bundled with Claude Desktop (if available).
4. **Custom Python Path**: Allows you to specify a custom Python executable path.

## Manual Configuration

For manual setup, follow these steps:

### 1. Install Required Python Package

Install the `mcp` package with the following command:

```bash
python -m pip install mcp>=0.1.0
```

### 2. Create a Run Script

Create a batch file named `run_unreal_architect.bat` with this content:

```batch
@echo off
setlocal
cd /d "%~dp0"
python "%~dp0unreal_mcp_server.py"
```

Save it in the `Plugins\UnrealArchitect\MCP\` directory.

### 3. Configure Claude Desktop

Locate or create the Claude Desktop configuration file at:

```
%APPDATA%\Claude\claude_desktop_config.json
```

Add or update it with the following content, replacing the path with the actual location of your `run_unreal_architect.bat`:

```json
{
    "mcpServers": {
        "unreal": {
            "command": "C:\\Path\\To\\Your\\Plugins\\UnrealArchitect\\MCP\\run_unreal_architect.bat",
            "args": []
        }
    }
}
```

## Troubleshooting

### Common Issues

1. **"No module named 'mcp'"**
   - **Cause**: The `mcp` package isn’t installed in the Python environment used by Claude Desktop.
   - **Solution**: Rerun the `setup_unreal_architect.bat` script and select the correct Python environment.

2. **Connection refused errors**
   - **Cause**: The MCP server isn’t running or isn’t listening on port 13377.
   - **Solution**:
     - Ensure Unreal Engine is running with the MCP plugin enabled.
     - Confirm the MCP plugin’s port setting matches the default (13377).

3. **Claude Desktop can’t start the MCP server**
   - **Cause**: Configuration or file path issues.
   - **Solution**:
     - Check the logs at: `%APPDATA%\Claude\logs\mcp-server-unreal.log`
     - Verify the path in `claude_desktop_config.json` is correct.
     - Ensure `run_unreal_architect.bat` exists and references the correct Python interpreter.

### Checking Logs

Claude Desktop logs MCP server output to:

```
%APPDATA%\Claude\logs\mcp-server-unreal.log
```

Review this file for detailed error messages.

## Usage

To use the MCP interface:

1. Launch your Unreal Engine project with the MCP plugin enabled.
2. Open Claude Desktop.
3. Use natural language commands in Claude Desktop, such as:
   - "Show me what's in the current Unreal scene"
   - "Create a cube at position [0, 0, 100]"
   - "Modify the object named 'Cube_1' to have scale [2, 2, 2]"
   - "Delete the object named 'Cube_1'"

## Available Commands

The MCP interface supports these commands:

- **`get_scene_info`**: Retrieves details about the current scene.
- **`create_object`**: Spawns a new object in the scene.
- **`modify_object`**: Updates properties of an existing object.
- **`delete_object`**: Removes an object from the scene.

## Testing the MCP Server Directly

To test the MCP server independently of Claude Desktop:

1. Run the following script:
   ```
   Plugins\UnrealArchitect\MCP\run_unreal_architect.bat
   ```

This starts the MCP server using the configured Python interpreter, allowing it to listen for connections.
```