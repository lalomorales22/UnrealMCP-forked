"""Test script for UnrealMCP basic commands.

This script tests the basic scene and Python execution commands available in the UnrealMCP bridge.
Make sure Unreal Engine is running with the UnrealMCP plugin enabled before running this script.
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

def test_scene_info():
    """Test getting scene information."""
    print("\n1. Testing get_scene_info...")
    try:
        response = send_command("get_scene_info")
        print(f"Get Scene Info Response: {json.dumps(response, indent=2)}")
        return response["status"] == "success"
    except Exception as e:
        print(f"Error testing get_scene_info: {e}")
        return False

def test_object_creation():
    """Test creating objects in the scene."""
    print("\n2. Testing create_object...")
    try:
        params = {
            "type": "StaticMeshActor",
            "location": [0, 0, 100],
            "label": "TestCube"
        }
        response = send_command("create_object", params)
        print(f"Create Object Response: {json.dumps(response, indent=2)}")
        return response["status"] == "success"
    except Exception as e:
        print(f"Error testing create_object: {e}")
        return False

def test_python_execution():
    """Test Python execution in Unreal Engine."""
    print("\n3. Testing execute_python...")
    
    test_code = """
import unreal
print("Python executing in Unreal Engine!")
world = unreal.EditorLevelLibrary.get_editor_world()
print(f"Current level: {world.get_name()}")
actors = unreal.EditorLevelLibrary.get_all_level_actors()
print(f"Number of actors in level: {len(actors)}")
"""
    
    try:
        response = send_command("execute_python", {"code": test_code})
        print(f"Python Execution Response: {json.dumps(response, indent=2)}")
        return response["status"] == "success"
    except Exception as e:
        print(f"Error testing Python execution: {e}")
        return False

def main():
    """Run all basic command tests."""
    print("Starting UnrealMCP basic command tests...")
    print("Make sure Unreal Engine is running with the UnrealMCP plugin enabled!")
    
    try:
        results = {
            "get_scene_info": test_scene_info(),
            "create_object": test_object_creation(),
            "execute_python": test_python_execution()
        }
        
        print("\nTest Results:")
        print("-" * 40)
        for test_name, success in results.items():
            status = "✓ PASS" if success else "✗ FAIL"
            print(f"{status} - {test_name}")
        print("-" * 40)
        
        if all(results.values()):
            print("\nAll basic tests passed successfully!")
        else:
            print("\nSome tests failed. Check the output above for details.")
            sys.exit(1)
            
    except Exception as e:
        print(f"\nError during testing: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main() 