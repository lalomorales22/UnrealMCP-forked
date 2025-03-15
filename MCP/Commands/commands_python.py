"""Python execution commands for Unreal Engine.

This module contains commands for executing Python code in Unreal Engine.
"""

import sys
import os
from mcp.server.fastmcp import Context

# Import send_command from the parent module
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from unreal_mcp_bridge import send_command

def register_all(mcp):
    """Register all Python execution commands with the MCP server."""
    
    @mcp.tool()
    def execute_python(ctx: Context, code: str = None, file: str = None) -> str:
        """Execute Python code or a Python script file in Unreal Engine.
        
        This function allows you to execute arbitrary Python code directly in the Unreal Engine
        environment. You can either provide Python code as a string or specify a path to a Python
        script file to execute.
        
        The Python code will have access to the full Unreal Engine Python API, including the 'unreal'
        module, allowing you to interact with and manipulate the Unreal Engine editor and its assets.
        
        Args:
            code: Python code to execute as a string. Can be multiple lines.
            file: Path to a Python script file to execute.
            
        Note: 
            - You must provide either code or file, but not both.
            - The output of the Python code will be visible in the Unreal Engine log.
            - The Python code runs in the Unreal Engine process, so it has full access to the engine.
            - Be careful with destructive operations as they can affect your project.
            
        Examples:
            # Execute simple Python code
            execute_python(code="print('Hello from Unreal Engine!')")
            
            # Get information about the current level
            execute_python(code='''
            import unreal
            level = unreal.EditorLevelLibrary.get_editor_world()
            print(f"Current level: {level.get_name()}")
            actors = unreal.EditorLevelLibrary.get_all_level_actors()
            print(f"Number of actors: {len(actors)}")
            ''')
            
            # Execute a Python script file
            execute_python(file="D:/my_scripts/create_assets.py")
        """
        try:
            if not code and not file:
                return "Error: You must provide either 'code' or 'file' parameter"
            
            if code and file:
                return "Error: You can only provide either 'code' or 'file', not both"
            
            params = {}
            if code:
                params["code"] = code
            if file:
                params["file"] = file
                
            response = send_command("execute_python", params)
            
            # Handle the response
            if response["status"] == "success":
                return f"Python execution successful:\n{response['result']['output']}"
            elif response["status"] == "error":
                # New format with detailed error information
                result = response.get("result", {})
                output = result.get("output", "")
                error = result.get("error", "")
                
                # Format the response with both output and error information
                response_text = "Python execution failed with errors:\n\n"
                
                if output:
                    response_text += f"--- Output ---\n{output}\n\n"
                    
                if error:
                    response_text += f"--- Error ---\n{error}"
                    
                return response_text
            else:
                return f"Error: {response['message']}"
        except Exception as e:
            return f"Error executing Python: {str(e)}" 