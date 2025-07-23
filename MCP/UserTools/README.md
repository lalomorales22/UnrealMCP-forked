
### User Guide: Adding Custom MCP Tools

To extend the functionality of the UnrealArchitect plugin with your own tools, follow these steps:

1. **Locate the `user_tools` Directory**
   - Find the `user_tools` directory in the plugin’s MCP folder (e.g., `Plugins/UnrealArchitect/MCP/user_tools`).
   - If it doesn’t exist, create it manually.

2. **Create a Python Script**
   - Add a new `.py` file in the `user_tools` directory (e.g., `my_tool.py`).
   - Define a `register_tools(mcp, utils)` function in your script to register your custom tools.

3. **Define Your Tools**
   - Use the `@mcp.tool()` decorator to create tools.
   - Access the `send_command` function via `utils['send_command']` to interact with Unreal Engine.
   - Example:

     ```python
     def register_tools(mcp, utils):
         send_command = utils['send_command']
         
         @mcp.tool()
         def create_cube(ctx, location: list) -> str:
             """Create a cube at the specified location."""
             code = f"""
             import unreal
             location = unreal.Vector({location[0]}, {location[1]}, {location[2]})
             unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.StaticMeshActor, location)
             """
             response = send_command("execute_python", {"code": code})
             return "Cube created" if response["status"] == "success" else f"Error: {response['message']}"
     ```

4. **Run the Bridge**
   - Start the MCP bridge as usual with `run_unreal_mcp.bat`. Your tools will be loaded automatically.

**Notes:**
- Tools run in the bridge’s Python process and communicate with Unreal Engine via the MCP server.
- Use `send_command("execute_python", {"code": "..."})` to execute Python code in Unreal Engine’s interpreter, accessing the `unreal` module.
- Ensure any additional Python packages required by your tools are installed in Unreal Engine’s Python environment (not the bridge’s virtual environment) if using `execute_python`.
