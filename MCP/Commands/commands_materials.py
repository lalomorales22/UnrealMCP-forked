"""Material-related commands for Unreal Engine.

This module contains all material-related commands for the UnrealArchitect bridge,
including creation, modification, and querying of materials.
"""

import sys
import os
import importlib.util
import importlib
from mcp.server.fastmcp import Context

# Import send_command from the parent module
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from unreal_mcp_bridge import send_command

def register_all(mcp):
    """Register all material-related commands with the MCP server."""
    
    # Create material command
    @mcp.tool()
    def create_material(ctx: Context, package_path: str, name: str, properties: dict = None) -> str:
        """Create a new material in the Unreal project.
        
        Args:
            package_path: The path where the material should be created (e.g., '/Game/Materials')
            name: The name of the material
            properties: Optional dictionary of material properties to set. Can include:
                - shading_model: str (e.g., "DefaultLit", "Unlit", "Subsurface", etc.)
                - blend_mode: str (e.g., "Opaque", "Masked", "Translucent", etc.)
                - two_sided: bool
                - dithered_lod_transition: bool
                - cast_contact_shadow: bool
                - base_color: list[float] (RGBA values 0-1)
                - metallic: float (0-1)
                - roughness: float (0-1)
        """
        try:
            params = {
                "package_path": package_path,
                "name": name
            }
            if properties:
                params["properties"] = properties
            response = send_command("create_material", params)
            if response["status"] == "success":
                return f"Created material: {response['result']['name']} at path: {response['result']['path']}"
            else:
                return f"Error: {response['message']}"
        except Exception as e:
            return f"Error creating material: {str(e)}"

    # Modify material command
    @mcp.tool()
    def modify_material(ctx: Context, path: str, properties: dict) -> str:
        """Modify an existing material's properties.
        
        Args:
            path: The full path to the material (e.g., '/Game/Materials/MyMaterial')
            properties: Dictionary of material properties to set. Can include:
                - shading_model: str (e.g., "DefaultLit", "Unlit", "Subsurface", etc.)
                - blend_mode: str (e.g., "Opaque", "Masked", "Translucent", etc.)
                - two_sided: bool
                - dithered_lod_transition: bool
                - cast_contact_shadow: bool
                - base_color: list[float] (RGBA values 0-1)
                - metallic: float (0-1)
                - roughness: float (0-1)
        """
        try:
            params = {
                "path": path,
                "properties": properties
            }
            response = send_command("modify_material", params)
            if response["status"] == "success":
                return f"Modified material: {response['result']['name']} at path: {response['result']['path']}"
            else:
                return f"Error: {response['message']}"
        except Exception as e:
            return f"Error modifying material: {str(e)}"

    # Get material info command
    @mcp.tool()
    def get_material_info(ctx: Context, path: str) -> dict:
        """Get information about a material.
        
        Args:
            path: The full path to the material (e.g., '/Game/Materials/MyMaterial')
            
        Returns:
            Dictionary containing material information including:
                - name: str
                - path: str
                - shading_model: str
                - blend_mode: str
                - two_sided: bool
                - dithered_lod_transition: bool
                - cast_contact_shadow: bool
                - base_color: list[float]
                - metallic: float
                - roughness: float
        """
        try:
            params = {"path": path}
            response = send_command("get_material_info", params)
            if response["status"] == "success":
                return response["result"]
            else:
                return {"error": response["message"]}
        except Exception as e:
            return {"error": str(e)} 