#!/usr/bin/env python
import socket
import json
import time
import sys

def simple_test(host="localhost", port=1337):
    """Connect to the server, send a command, and disconnect."""
    try:
        # Create socket
        print(f"Connecting to {host}:{port}...")
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(10)  # 10 second timeout
        
        # Connect to server
        s.connect((host, port))
        print("Connected successfully")
        
        # Send a simple command
        command = {
            "type": "get_scene_info",
            "params": {}
        }
        
        print(f"Sending command: {json.dumps(command)}")
        s.sendall(json.dumps(command).encode('utf-8'))
        print("Command sent")
        
        # Wait for response
        print("Waiting for response...")
        response = s.recv(8192)
        print(f"Received {len(response)} bytes")
        
        if response:
            try:
                response_data = json.loads(response.decode('utf-8'))
                print(f"Response status: {response_data.get('status', 'unknown')}")
                print("Test successful")
                return True
            except json.JSONDecodeError:
                print(f"Raw response (not valid JSON): {response}")
                print("Test failed - invalid JSON response")
                return False
        else:
            print("No response received (connection closed by server)")
            print("Test failed - no response")
            return False
    except ConnectionRefusedError:
        print("Connection refused. Is the server running?")
        print("Test failed - connection refused")
        return False
    except Exception as e:
        print(f"Error: {e}")
        print("Test failed - exception")
        return False
    finally:
        # Close the socket
        try:
            s.close()
            print("Socket closed")
        except:
            pass

if __name__ == "__main__":
    print("=== Simple MCP Server Test ===")
    if len(sys.argv) > 1:
        host = sys.argv[1]
    else:
        host = "localhost"
        
    if len(sys.argv) > 2:
        port = int(sys.argv[2])
    else:
        port = 1337
        
    print(f"Host: {host}")
    print(f"Port: {port}")
    
    if simple_test(host, port):
        sys.exit(0)
    else:
        sys.exit(1) 