#!/usr/bin/env python
import socket
import json
import sys
import os

def main():
    """Test the execute_python command."""
    try:
        # Create socket
        print("Connecting to localhost:1337...")
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(10)  # 10 second timeout
        
        # Connect to server
        s.connect(("localhost", 1337))
        print("Connected successfully")
        
        # Test executing Python code
        print("\n=== Testing Python code execution ===")
        code = """
import unreal
import sys

# Get the current level
level = unreal.EditorLevelLibrary.get_editor_world()
print(f"Current level: {level.get_name()}")

# Get all actors in the level
actors = unreal.EditorLevelLibrary.get_all_level_actors()
print(f"Number of actors in level: {len(actors)}")

# Print Python version
print(f"Python version: {sys.version}")

# A cheeky little print statement
print("I'm not saying Unreal is better than Unity... but have you seen these graphics? Just saying!")
"""
        
        command = {
            "type": "execute_python",
            "params": {
                "code": code
            }
        }
        
        print(f"Sending execute_python command with code...")
        s.sendall(json.dumps(command).encode('utf-8'))
        print("Command sent")
        
        # Wait for response
        print("Waiting for response...")
        response_data = b''
        while True:
            chunk = s.recv(8192)
            if not chunk:
                break
            response_data += chunk
            try:
                # Try to parse as JSON to see if we have a complete response
                json.loads(response_data.decode('utf-8'))
                break
            except json.JSONDecodeError:
                # Not complete yet, continue receiving
                continue
        
        if response_data:
            try:
                response = json.loads(response_data.decode('utf-8'))
                print(f"Response status: {response.get('status', 'unknown')}")
                if response.get('status') == 'success':
                    print(f"Output:\n{response.get('result', {}).get('output', 'No output')}")
                else:
                    print(f"Error: {response.get('message', 'Unknown error')}")
            except json.JSONDecodeError:
                print(f"Raw response (not valid JSON): {response_data}")
        else:
            print("No response received")
        
        # Close the socket
        s.close()
        print("Socket closed")
        
    except ConnectionRefusedError:
        print("Connection refused. Is the server running?")
        return False
    except Exception as e:
        print(f"Error: {e}")
        return False
    
    return True

if __name__ == "__main__":
    print("=== Python Execution Test ===")
    if main():
        print("Test completed successfully")
        sys.exit(0)
    else:
        print("Test failed")
        sys.exit(1) 