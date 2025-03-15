"""Scene-related commands for Unreal Engine.

This module contains all scene-related commands for the UnrealMCP bridge,
including getting scene information, creating, modifying, and deleting objects.
"""

import sys
import os
from mcp.server.fastmcp import Context

# Import send_command from the parent module
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from unreal_mcp_bridge import send_command

def register_all(mcp):
    """Register all scene-related commands with the MCP server."""
    
    @mcp.tool()
    def get_scene_info(ctx: Context) -> str:
        """Get detailed information about the current Unreal scene."""
        try:
            response = send_command("get_scene_info")
            if response["status"] == "success":
                return json.dumps(response["result"], indent=2)
            else:
                return f"Error: {response['message']}"
        except Exception as e:
            return f"Error getting scene info: {str(e)}"

    @mcp.tool()
    def create_object(ctx: Context, type: str, location: list = None, label: str = None) -> str:
        """Create a new object in the Unreal scene.
        
        Args:
            type: The type of object to create (e.g., 'StaticMeshActor', 'PointLight', etc.)
            location: Optional 3D location as [x, y, z]
            label: Optional label for the object
        """
        try:
            params = {"type": type}
            if location:
                params["location"] = location
            if label:
                params["label"] = label
            response = send_command("create_object", params)
            if response["status"] == "success":
                return f"Created object: {response['result']['name']} with label: {response['result']['label']}"
            else:
                return f"Error: {response['message']}"
        except Exception as e:
            return f"Error creating object: {str(e)}"

    @mcp.tool()
    def modify_object(ctx: Context, name: str, location: list = None, rotation: list = None, scale: list = None) -> str:
        """Modify an existing object in the Unreal scene.
        
        Args:
            name: The name of the object to modify
            location: Optional 3D location as [x, y, z]
            rotation: Optional rotation as [pitch, yaw, roll]
            scale: Optional scale as [x, y, z]
        """
        try:
            params = {"name": name}
            if location:
                params["location"] = location
            if rotation:
                params["rotation"] = rotation
            if scale:
                params["scale"] = scale
            response = send_command("modify_object", params)
            if response["status"] == "success":
                return f"Modified object: {response['result']['name']}"
            else:
                return f"Error: {response['message']}"
        except Exception as e:
            return f"Error modifying object: {str(e)}"

    @mcp.tool()
    def delete_object(ctx: Context, name: str) -> str:
        """Delete an object from the Unreal scene.
        
        Args:
            name: The name of the object to delete
        """
        try:
            response = send_command("delete_object", {"name": name})
            if response["status"] == "success":
                return f"Deleted object: {name}"
            else:
                return f"Error: {response['message']}"
        except Exception as e:
            return f"Error deleting object: {str(e)}" 