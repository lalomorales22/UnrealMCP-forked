#!/usr/bin/env python
import socket
import json
import time
import sys

def send_command(command_type, params=None):
    """Send a command to the C++ MCP server and return the response."""
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.settimeout(10)  # Set a 10-second timeout
            print(f"Connecting to localhost:9876...")
            s.connect(("localhost", 9876))
            print("Connected successfully")
            
            command = {
                "type": command_type,
                "params": params or {}
            }
            
            print(f"Sending command: {json.dumps(command, indent=2)}")
            s.sendall(json.dumps(command).encode('utf-8'))
            
            # Read response with a buffer
            chunks = []
            response_data = b''
            
            # Wait for data with timeout
            while True:
                try:
                    chunk = s.recv(8192)
                    if not chunk:  # Connection closed
                        print("Connection closed by server")
                        break
                    
                    print(f"Received {len(chunk)} bytes")
                    chunks.append(chunk)
                    
                    # Try to parse what we have so far
                    response_data = b''.join(chunks)
                    try:
                        # If we can parse it as JSON, we have a complete response
                        json.loads(response_data.decode('utf-8'))
                        print("Received complete JSON response")
                        break
                    except json.JSONDecodeError:
                        # Incomplete JSON, continue receiving
                        print("Incomplete JSON, continuing to receive...")
                        continue
                except socket.timeout:
                    print("Socket timeout while receiving data")
                    # If we have some data but timed out, try to use what we have
                    if response_data:
                        break
                    raise
            
            if not response_data:
                raise Exception("No data received from server")
                
            response = json.loads(response_data.decode('utf-8'))
            print(f"Parsed response: {json.dumps(response, indent=2)}")
            return response
            
    except ConnectionRefusedError:
        print("Error: Could not connect to Unreal MCP server on localhost:9876.")
        print("Make sure your Unreal Engine with MCP plugin is running.")
        return None
    except socket.timeout:
        print("Error: Connection timed out while communicating with Unreal MCP server.")
        return None
    except Exception as e:
        print(f"Error communicating with Unreal MCP server: {str(e)}")
        return None

def main():
    print("Testing robust connection to Unreal MCP server")
    
    # Test get_scene_info
    print("\n=== Testing get_scene_info ===")
    response = send_command("get_scene_info")
    if not response:
        print("Failed to get scene info")
        return
    
    # Test create_object
    print("\n=== Testing create_object (cube) ===")
    response = send_command("create_object", {
        "type": "CUBE",
        "location": [100, 0, 0]
    })
    if not response:
        print("Failed to create cube")
        return
    
    # Wait a bit
    print("Waiting 1 second...")
    time.sleep(1)
    
    # Test get_scene_info again
    print("\n=== Testing get_scene_info again ===")
    response = send_command("get_scene_info")
    if not response:
        print("Failed to get scene info")
        return
    
    # Find the cube we created
    if "result" in response and "actors" in response["result"]:
        actors = response["result"]["actors"]
        for actor in actors:
            if "type" in actor and "StaticMeshActor" in actor["type"]:
                actor_name = actor["name"]
                print(f"Found actor: {actor_name}")
                
                # Test modify_object
                print(f"\n=== Testing modify_object ({actor_name}) ===")
                response = send_command("modify_object", {
                    "name": actor_name,
                    "location": [50, 50, 50],
                    "rotation": [0, 45, 0],
                    "scale": [2, 2, 2]
                })
                if not response:
                    print("Failed to modify object")
                    return
                
                # Wait a bit
                print("Waiting 1 second...")
                time.sleep(1)
                
                # Test delete_object
                print(f"\n=== Testing delete_object ({actor_name}) ===")
                response = send_command("delete_object", {
                    "name": actor_name
                })
                if not response:
                    print("Failed to delete object")
                    return
                
                print("Test completed successfully!")
                return
    
    print("Could not find the created actor in the scene")

if __name__ == "__main__":
    main() 