#!/usr/bin/env python
import socket
import json
import time
import sys

def send_command(sock, command_type, params=None):
    """Send a command to the MCP server and return the response."""
    if params is None:
        params = {}
    
    command = {
        "type": command_type,
        "params": params
    }
    
    print(f"Sending command: {json.dumps(command, indent=2)}")
    sock.sendall(json.dumps(command).encode('utf-8'))
    
    # Wait for response
    response = sock.recv(8192)
    response_data = json.loads(response.decode('utf-8'))
    print(f"Received response: {json.dumps(response_data, indent=2)}")
    return response_data

def main():
    # Connect to the MCP server
    port = 9876
    if len(sys.argv) > 1:
        port = int(sys.argv[1])
    
    print(f"Connecting to MCP server on localhost:{port}")
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    
    try:
        sock.connect(("localhost", port))
        print("Connected to MCP server")
        
        # Get scene info
        send_command(sock, "get_scene_info")
        
        # Create a cube
        send_command(sock, "create_object", {
            "type": "CUBE",
            "location": [100, 0, 0]
        })
        
        # Create a sphere
        send_command(sock, "create_object", {
            "type": "SPHERE",
            "location": [0, 100, 0]
        })
        
        # Wait a bit
        time.sleep(1)
        
        # Get scene info again to see the new objects
        response = send_command(sock, "get_scene_info")
        
        # If we have actors, modify the first one
        if "result" in response and "actors" in response["result"] and len(response["result"]["actors"]) > 0:
            actor_name = response["result"]["actors"][0]["name"]
            print(f"Modifying actor: {actor_name}")
            
            send_command(sock, "modify_object", {
                "name": actor_name,
                "location": [50, 50, 50],
                "rotation": [0, 45, 0],
                "scale": [2, 2, 2]
            })
            
            # Wait a bit
            time.sleep(1)
            
            # Delete the actor
            send_command(sock, "delete_object", {
                "name": actor_name
            })
        
    except Exception as e:
        print(f"Error: {e}")
    finally:
        sock.close()
        print("Disconnected from MCP server")

if __name__ == "__main__":
    main() 