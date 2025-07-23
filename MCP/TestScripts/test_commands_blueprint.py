"""Test script for UnrealArchitect blueprint commands.

This script tests the blueprint-related commands available in the UnrealArchitect bridge.
Make sure Unreal Engine is running with the UnrealArchitect plugin enabled before running this script.
"""

import sys
import os
import json
import time
from mcp.server.fastmcp import FastMCP, Context

# Add the MCP directory to sys.path so we can import unreal_mcp_bridge
mcp_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if mcp_dir not in sys.path:
    sys.path.insert(0, mcp_dir)

from unreal_mcp_bridge import send_command

# Global variables to store paths
blueprint_path = ""
# Longer timeout for Unreal Engine operations
TIMEOUT = 30

def test_create_blueprint():
    """Test blueprint creation command."""
    global blueprint_path
    print("\n1. Testing create_blueprint...")
    try:
        # Define the package path and blueprint name
        # Use a subdirectory to ensure proper directory structure
        package_path = "/Game/Blueprints/TestDir"
        blueprint_name = "TestBlueprint"
        
        # Print the expected file path (this is just an approximation)
        project_dir = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
        # Go up one more level to get to the actual project directory
        project_dir = os.path.dirname(project_dir)
        project_content_dir = os.path.join(project_dir, "Content")
        
        # Check both possible locations based on how the path is interpreted
        expected_path_in_dir = os.path.join(project_content_dir, "Blueprints", "TestDir", f"{blueprint_name}.uasset")
        expected_path_as_asset = os.path.join(project_content_dir, "Blueprints", "TestDir.uasset")
        
        print(f"Project directory: {project_dir}")
        print(f"Project Content directory: {project_content_dir}")
        print(f"Expected file path (in directory): {expected_path_in_dir}")
        print(f"Expected file path (as asset): {expected_path_as_asset}")
        
        # Print debug info about the current working directory
        print(f"Current working directory: {os.getcwd()}")
        
        # The package_path should be the directory, and name should be the asset name
        params = {
            "package_path": package_path,  # Directory path
            "name": blueprint_name,        # Asset name
            "properties": {
                "parent_class": "Actor"    # Default parent class
            }
        }
        
        print(f"Sending create_blueprint command with package_path={params['package_path']} and name={params['name']}")
        response = send_command("create_blueprint", params, timeout=TIMEOUT)
        print(f"Create Blueprint Response: {json.dumps(response, indent=2)}")
        
        # Store the actual path from the response for later tests
        if response["status"] == "success":
            blueprint_path = response["result"]["path"]
            file_path = response["result"].get("file_path")
            print(f"Blueprint created at: {blueprint_path}")
            if file_path and os.path.exists(file_path):
                print(f"\u2713 Blueprint file found at: {file_path}")
            else:
                print("\u2717 Blueprint file not found at reported path")
            
        return response["status"] == "success"
    except Exception as e:
        print(f"Error testing create_blueprint: {e}")
        return False

def test_get_blueprint_info():
    """Test getting blueprint information."""
    global blueprint_path
    print("\n2. Testing get_blueprint_info...")
    try:
        # Use the path from the create_blueprint response
        params = {
            "blueprint_path": blueprint_path
        }
        response = send_command("get_blueprint_info", params, timeout=TIMEOUT)
        print(f"Get Blueprint Info Response: {json.dumps(response, indent=2)}")
        return response["status"] == "success"
    except Exception as e:
        print(f"Error testing get_blueprint_info: {e}")
        return False

def test_create_blueprint_event():
    """Test creating a blueprint event."""
    global blueprint_path
    print("\n3. Testing create_blueprint_event...")
    try:
        # Use the path from the create_blueprint response
        params = {
            "event_name": "TestEvent",
            "blueprint_path": blueprint_path
        }
        
        # Set a longer timeout for this operation
        print("This operation may take some time...")
        response = send_command("create_blueprint_event", params, timeout=TIMEOUT)
        print(f"Create Blueprint Event Response: {json.dumps(response, indent=2)}")
        return response["status"] == "success"
    except Exception as e:
        print(f"Error testing create_blueprint_event: {e}")
        return False

def test_modify_blueprint():
    """Test modifying a blueprint."""
    global blueprint_path
    print("\n4. Testing modify_blueprint...")
    try:
        # Use the path from the create_blueprint response
        params = {
            "blueprint_path": blueprint_path,
            "properties": {
                "description": "A test blueprint created by MCP",
                "category": "Tests",
                "options": {
                    "hide_categories": ["Variables", "Transformation"],
                    "namespace": "MCP",
                    "display_name": "MCP Test Blueprint",
                    "compile_mode": "Development",
                    "abstract_class": False,
                    "const_class": False,
                    "deprecate": False
                }
            }
        }
        response = send_command("modify_blueprint", params, timeout=TIMEOUT)
        print(f"Modify Blueprint Response: {json.dumps(response, indent=2)}")
        
        # Verify the changes by getting the blueprint info again
        if response["status"] == "success":
            print("\nVerifying blueprint modifications...")
            verify_params = {
                "blueprint_path": blueprint_path
            }
            verify_response = send_command("get_blueprint_info", verify_params, timeout=TIMEOUT)
            print(f"Updated Blueprint Info: {json.dumps(verify_response, indent=2)}")
            
            # Check if the events were updated
            if verify_response["status"] == "success":
                result = verify_response["result"]
                
                # Check for events
                if "events" in result and len(result["events"]) > 0:
                    print(f"✓ Blueprint has {len(result['events'])} events")
                    
                    # Look for our TestEvent
                    test_event_found = False
                    for event in result["events"]:
                        if "name" in event and "TestEvent" in event["name"]:
                            test_event_found = True
                            print(f"✓ Found TestEvent: {event['name']}")
                            break
                    
                    if not test_event_found:
                        print("✗ TestEvent not found in events")
                else:
                    print("✗ No events found in blueprint")
        
        return response["status"] == "success"
    except Exception as e:
        print(f"Error testing modify_blueprint: {e}")
        return False

def main():
    """Run all blueprint-related tests."""
    print("Starting UnrealArchitect blueprint command tests...")
    print("Make sure Unreal Engine is running with the UnrealArchitect plugin enabled!")
    
    try:
        # Run tests in sequence, with each test depending on the previous one
        create_result = test_create_blueprint()
        
        # Only run subsequent tests if the blueprint was created successfully
        if create_result:
            # Wait a moment for the blueprint to be fully created
            time.sleep(1)
            get_info_result = test_get_blueprint_info()
            
            # Only run event creation if get_info succeeded
            if get_info_result:
                create_event_result = test_create_blueprint_event()
            else:
                create_event_result = False
                
            # Only run modify if previous tests succeeded
            if create_event_result:
                modify_result = test_modify_blueprint()
            else:
                modify_result = False
        else:
            get_info_result = False
            create_event_result = False
            modify_result = False
        
        results = {
            "create_blueprint": create_result,
            "get_blueprint_info": get_info_result,
            "create_blueprint_event": create_event_result,
            "modify_blueprint": modify_result
        }
        
        print("\nTest Results:")
        print("-" * 40)
        for test_name, success in results.items():
            status = "✓ PASS" if success else "✗ FAIL"
            print(f"{status} - {test_name}")
        print("-" * 40)
        
        if all(results.values()):
            print("\nAll blueprint tests passed successfully!")
        else:
            print("\nSome tests failed. Check the output above for details.")
            sys.exit(1)
            
    except Exception as e:
        print(f"\nError during testing: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main() 