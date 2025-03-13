#!/usr/bin/env python3
"""
Simple Test Command for MCP Server

This script sends a very simple Python command to the MCP Server.
"""

import socket
import json
import sys

def main():
    """Send a simple Python command to the MCP Server."""
    try:
        # Create socket
        print("Connecting to MCP Server on localhost:1337...")
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(5)  # 5 second timeout
        
        # Connect to server
        s.connect(("localhost", 1337))
        print("Connected successfully")
        
        # Very simple Python code
        code = 'import unreal\nreturn "Hello from Python!"'
        
        # Try different command formats
        formats = [
            {
                "name": "Format 1",
                "command": {
                    "type": "execute_python",
                    "code": code
                }
            },
            {
                "name": "Format 2",
                "command": {
                    "command": "execute_python",
                    "code": code
                }
            },
            {
                "name": "Format 3",
                "command": {
                    "type": "execute_python",
                    "data": {
                        "code": code
                    }
                }
            },
            {
                "name": "Format 4",
                "command": {
                    "command": "execute_python",
                    "type": "execute_python",
                    "code": code
                }
            },
            {
                "name": "Format 5",
                "command": {
                    "command": "execute_python",
                    "type": "execute_python",
                    "data": {
                        "code": code
                    }
                }
            }
        ]
        
        # Test each format
        for format_info in formats:
            format_name = format_info["name"]
            command = format_info["command"]
            
            print(f"\n=== Testing {format_name} ===")
            print(f"Command: {json.dumps(command)}")
            
            # Create a new socket for each test
            test_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            test_socket.settimeout(5)
            test_socket.connect(("localhost", 1337))
            
            # Send command with newline
            command_str = json.dumps(command) + "\n"
            test_socket.sendall(command_str.encode('utf-8'))
            
            # Receive response
            response = b""
            while True:
                data = test_socket.recv(4096)
                if not data:
                    break
                response += data
                if b"\n" in data:
                    break
            
            # Close socket
            test_socket.close()
            
            # Process response
            if response:
                response_str = response.decode('utf-8').strip()
                print(f"Response: {response_str}")
                
                try:
                    response_json = json.loads(response_str)
                    status = response_json.get('status', 'unknown')
                    
                    if status == 'success':
                        print(f"✓ {format_name} SUCCEEDED")
                        return True
                    else:
                        print(f"✗ {format_name} FAILED: {response_json.get('message', 'Unknown error')}")
                except json.JSONDecodeError as e:
                    print(f"✗ Error parsing JSON response: {e}")
            else:
                print("✗ No response received")
        
        print("\nAll formats failed.")
        return False
        
    except Exception as e:
        print(f"Error: {e}")
        return False

if __name__ == "__main__":
    print("=== Simple Test Command for MCP Server ===")
    success = main()
    sys.exit(0 if success else 1) 