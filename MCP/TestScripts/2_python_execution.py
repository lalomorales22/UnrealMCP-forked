#!/usr/bin/env python3
"""
Python Execution Test for MCP Server

This script tests executing Python code through the MCP Server.
It connects to the server, sends a Python code snippet, and verifies the execution.
"""

import socket
import json
import sys

def main():
    """Connect to the MCP Server and execute Python code."""
    try:
        # Create socket
        print("Connecting to MCP Server on localhost:1337...")
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(5)  # 5 second timeout
        
        # Connect to server
        s.connect(("localhost", 1337))
        print("✓ Connected successfully")
        
        # Python code to execute
        code = """
import unreal

# Get the current level
level = unreal.EditorLevelLibrary.get_editor_world()
level_name = level.get_name()

# Get all actors in the level
actors = unreal.EditorLevelLibrary.get_all_level_actors()
actor_count = len(actors)

# Log some information
unreal.log(f"Current level: {level_name}")
unreal.log(f"Actor count: {actor_count}")

# Return a result
return {
    "level_name": level_name,
    "actor_count": actor_count
}
"""
        
        # Create command
        command = {
            "type": "execute_python",
            "code": code
        }
        
        # Send command
        print("Sending execute_python command...")
        command_str = json.dumps(command) + "\n"  # Add newline
        s.sendall(command_str.encode('utf-8'))
        
        # Receive response
        print("Waiting for response...")
        response = b""
        while True:
            data = s.recv(4096)
            if not data:
                break
            response += data
            if b"\n" in data:  # Check for newline which indicates end of response
                break
        
        # Close connection
        s.close()
        print("✓ Connection closed properly")
        
        # Process response
        if response:
            response_str = response.decode('utf-8').strip()
            
            try:
                response_json = json.loads(response_str)
                print("\n=== RESPONSE ===")
                print(f"Status: {response_json.get('status', 'unknown')}")
                
                if response_json.get('status') == 'success':
                    print("✓ Python execution successful")
                    result = response_json.get('result', {})
                    if isinstance(result, dict) and 'output' in result:
                        print(f"Output: {result['output']}")
                    return True
                else:
                    print("✗ Python execution failed")
                    print(f"Error: {response_json.get('message', 'Unknown error')}")
                    return False
            except json.JSONDecodeError as e:
                print(f"✗ Error parsing JSON response: {e}")
                print(f"Raw response: {response_str}")
                return False
        else:
            print("✗ No response received from server")
            return False
        
    except ConnectionRefusedError:
        print("✗ Connection refused. Is the MCP Server running?")
        return False
    except socket.timeout:
        print("✗ Connection timed out. Is the MCP Server running?")
        return False
    except Exception as e:
        print(f"✗ Error: {e}")
        return False

if __name__ == "__main__":
    print("=== MCP Server Python Execution Test ===")
    success = main()
    print("\n=== TEST RESULT ===")
    if success:
        print("✓ Python execution test PASSED")
        sys.exit(0)
    else:
        print("✗ Python execution test FAILED")
        sys.exit(1) 