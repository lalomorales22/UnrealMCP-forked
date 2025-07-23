# Unreal Architect

Unreal Architect exposes a Model Context Protocol (MCP) server inside Unreal Engine so external tools can automate the editor.

## Quick Start
1. Clone this repository into your project's `Plugins/UnrealArchitect` folder.
2. Regenerate project files and build the project.
3. Enable **Unreal Architect** and Unreal's **Python Editor Script Plugin** in the editor.
4. Run `MCP/setup_unreal_architect.bat` to install Python dependencies and generate `run_unreal_architect.bat`.
5. Start the server with `MCP/run_unreal_architect.bat` or let your MCP client launch it.

The server listens on `localhost:13377`. Connect with your MCP client (e.g. Claude Desktop) to issue commands.

## Repository Layout
- `Source/UnrealArchitect` – plugin source
- `MCP` – Python bridge and setup scripts
- `Resources` – icons and other assets

## License
MIT
