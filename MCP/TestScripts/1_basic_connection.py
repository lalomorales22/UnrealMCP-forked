#!/usr/bin/env python3
"""
Basic Connection Test for MCP Server

This script tests the basic connection to the MCP Server.
It connects to the server, sends a simple ping command, and verifies the response.
"""

import socket
import json
import sys

def main():
    """Connect to the MCP Server and verify the connection works."""
    try:
        # Create socket
        print("Connecting to MCP Server on localhost:1337...")
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(5)  # 5 second timeout
        
        # Connect to server
        s.connect(("localhost", 1337))
        print("✓ Connected successfully")
        
        # Create a simple get_scene_info command
        command = {
            "type": "get_scene_info"
        }
        
        # Send command
        print("Sending get_scene_info command...")
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
                    print("✓ Server responded successfully")
                    print(f"Level: {response_json.get('result', {}).get('level', 'unknown')}")
                    print(f"Actor count: {response_json.get('result', {}).get('actor_count', 0)}")
                    return True
                else:
                    print("✗ Server responded with an error")
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
    print("=== MCP Server Basic Connection Test ===")
    success = main()
    print("\n=== TEST RESULT ===")
    if success:
        print("✓ Connection test PASSED")
        sys.exit(0)
    else:
        print("✗ Connection test FAILED")
        sys.exit(1) 