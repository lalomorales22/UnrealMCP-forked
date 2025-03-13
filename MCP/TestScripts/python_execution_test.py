#!/usr/bin/env python
import socket
import json
import sys
import os

# Constants
DEFAULT_PORT = 1337
DEFAULT_BUFFER_SIZE = 32768  # 32KB buffer size
DEFAULT_TIMEOUT = 10  # 10 second timeout

def main():
    """Test the execute_python command."""
    try:
        # Create socket
        print(f"Connecting to localhost:{DEFAULT_PORT}...")
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(DEFAULT_TIMEOUT)  # timeout
        
        # Connect to server
        s.connect(("localhost", DEFAULT_PORT))
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
            chunk = s.recv(DEFAULT_BUFFER_SIZE)
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
                elif response.get('status') == 'error':
                    # Handle the new error response format
                    result = response.get('result', {})
                    output = result.get('output', '')
                    error = result.get('error', '')
                    
                    if output:
                        print(f"Output:\n{output}")
                    
                    if error:
                        print(f"Error:\n{error}")
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