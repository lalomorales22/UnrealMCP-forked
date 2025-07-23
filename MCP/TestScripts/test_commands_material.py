"""Test script for UnrealArchitect material commands.

This script tests the material-related commands available in the UnrealArchitect bridge.
Make sure Unreal Engine is running with the UnrealArchitect plugin enabled before running this script.
"""

import sys
import os
import json
from mcp.server.fastmcp import FastMCP, Context

# Add the MCP directory to sys.path so we can import unreal_mcp_bridge
mcp_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if mcp_dir not in sys.path:
    sys.path.insert(0, mcp_dir)

from unreal_mcp_bridge import send_command

def test_material_creation():
    """Test material creation command."""
    print("\n1. Testing create_material...")
    try:
        params = {
            "package_path": "/Game/Materials/Tests",
            "name": "TestMaterial",
            "properties": {
                "shading_model": "DefaultLit",
                "base_color": [1.0, 0.0, 0.0, 1.0],  # Red material
                "metallic": 0.0,
                "roughness": 0.5
            }
        }
        response = send_command("create_material", params)
        print(f"Create Material Response: {json.dumps(response, indent=2)}")
        return response["status"] == "success"
    except Exception as e:
        print(f"Error testing create_material: {e}")
        return False

def test_material_info():
    """Test getting material information."""
    print("\n2. Testing get_material_info...")
    try:
        params = {
            "path": "/Game/Materials/Tests/TestMaterial"
        }
        response = send_command("get_material_info", params)
        print(f"Get Material Info Response: {json.dumps(response, indent=2)}")
        return response["status"] == "success"
    except Exception as e:
        print(f"Error testing get_material_info: {e}")
        return False

def test_material_modification():
    """Test modifying material properties."""
    print("\n3. Testing modify_material...")
    try:
        params = {
            "path": "/Game/Materials/Tests/TestMaterial",
            "properties": {
                "base_color": [0.0, 1.0, 0.0, 1.0],  # Change to green
                "metallic": 0.5
            }
        }
        response = send_command("modify_material", params)
        print(f"Modify Material Response: {json.dumps(response, indent=2)}")
        return response["status"] == "success"
    except Exception as e:
        print(f"Error testing modify_material: {e}")
        return False

def main():
    """Run all material-related tests."""
    print("Starting UnrealArchitect material command tests...")
    print("Make sure Unreal Engine is running with the UnrealArchitect plugin enabled!")
    
    try:
        results = {
            "create_material": test_material_creation(),
            "get_material_info": test_material_info(),
            "modify_material": test_material_modification()
        }
        
        print("\nTest Results:")
        print("-" * 40)
        for test_name, success in results.items():
            status = "✓ PASS" if success else "✗ FAIL"
            print(f"{status} - {test_name}")
        print("-" * 40)
        
        if all(results.values()):
            print("\nAll material tests passed successfully!")
        else:
            print("\nSome tests failed. Check the output above for details.")
            sys.exit(1)
            
    except Exception as e:
        print(f"\nError during testing: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main() 