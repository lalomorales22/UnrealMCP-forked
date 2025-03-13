#!/usr/bin/env python
import socket
import json
import time
import sys
import random

def send_command(sock, command_type, params=None):
    """Send a command to the MCP server and return the response."""
    if params is None:
        params = {}
    
    command = {
        "type": command_type,
        "params": params
    }
    
    print(f"\n=== Sending command: {command_type} ===")
    print(f"Command: {json.dumps(command, indent=2)}")
    
    try:
        # Send the command
        sock.sendall(json.dumps(command).encode('utf-8'))
        print("Command sent successfully")
        
        # Wait for response
        response = sock.recv(8192)
        print(f"Received {len(response)} bytes")
        
        if response:
            response_data = json.loads(response.decode('utf-8'))
            print(f"Response: {json.dumps(response_data, indent=2)}")
            return response_data
        else:
            print("No response received (connection closed by server)")
            return None
    except Exception as e:
        print(f"Error: {e}")
        return None

def main():
    print("Multi-command test for Unreal MCP server")
    
    try:
        # Create socket
        print("Creating socket...")
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(10)  # 10 second timeout
        
        # Connect to server
        print("Connecting to localhost:9876...")
        s.connect(("localhost", 9876))
        print("Connected successfully")
        
        # Test 1: Get scene info
        response = send_command(s, "get_scene_info")
        if not response:
            print("Failed to get scene info, aborting test")
            s.close()
            return
        
        # Test 2: Create a cube
        cube_location = [random.randint(-200, 200), random.randint(-200, 200), random.randint(0, 200)]
        response = send_command(s, "create_object", {
            "type": "CUBE",
            "location": cube_location
        })
        if not response:
            print("Failed to create cube, aborting test")
            s.close()
            return
        
        cube_name = response.get("result", {}).get("name")
        if not cube_name:
            print("Cube name not found in response, aborting test")
            s.close()
            return
        
        print(f"Created cube: {cube_name}")
        
        # Wait a bit
        time.sleep(1)
        
        # Test 3: Create a sphere
        sphere_location = [random.randint(-200, 200), random.randint(-200, 200), random.randint(0, 200)]
        response = send_command(s, "create_object", {
            "type": "SPHERE",
            "location": sphere_location
        })
        if not response:
            print("Failed to create sphere, aborting test")
            s.close()
            return
        
        sphere_name = response.get("result", {}).get("name")
        if not sphere_name:
            print("Sphere name not found in response, aborting test")
            s.close()
            return
        
        print(f"Created sphere: {sphere_name}")
        
        # Wait a bit
        time.sleep(1)
        
        # Test 4: Modify the cube
        new_location = [random.randint(-200, 200), random.randint(-200, 200), random.randint(0, 200)]
        response = send_command(s, "modify_object", {
            "name": cube_name,
            "location": new_location,
            "rotation": [0, 45, 0],
            "scale": [2, 2, 2]
        })
        if not response:
            print("Failed to modify cube, aborting test")
            s.close()
            return
        
        print(f"Modified cube: {cube_name}")
        
        # Wait a bit
        time.sleep(1)
        
        # Test 5: Get scene info again
        response = send_command(s, "get_scene_info")
        if not response:
            print("Failed to get scene info, aborting test")
            s.close()
            return
        
        # Test 6: Delete the cube
        response = send_command(s, "delete_object", {
            "name": cube_name
        })
        if not response:
            print("Failed to delete cube, aborting test")
            s.close()
            return
        
        print(f"Deleted cube: {cube_name}")
        
        # Wait a bit
        time.sleep(1)
        
        # Test 7: Delete the sphere
        response = send_command(s, "delete_object", {
            "name": sphere_name
        })
        if not response:
            print("Failed to delete sphere, aborting test")
            s.close()
            return
        
        print(f"Deleted sphere: {sphere_name}")
        
        # Close the socket
        print("\nClosing socket...")
        s.close()
        print("Socket closed")
        
        print("\nAll tests completed successfully!")
        
    except ConnectionRefusedError:
        print("Connection refused. Is the Unreal MCP server running?")
    except Exception as e:
        print(f"Error: {e}")
        if 's' in locals():
            s.close()

if __name__ == "__main__":
    main() 