#!/usr/bin/env python
import socket
import json
import time
import sys

def connect_and_send_command():
    """Connect to the server, send a command, and then close the connection."""
    try:
        # Create socket
        print("\n=== Creating new connection ===")
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(5)  # 5 second timeout
        
        # Connect to server
        print("Connecting to localhost:9876...")
        s.connect(("localhost", 9876))
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
        try:
            response = s.recv(8192)
            print(f"Received {len(response)} bytes")
            
            if response:
                try:
                    response_data = json.loads(response.decode('utf-8'))
                    print(f"Response status: {response_data.get('status', 'unknown')}")
                    return True
                except json.JSONDecodeError:
                    print(f"Raw response (not valid JSON): {response}")
                    return False
            else:
                print("No response received (connection closed by server)")
                return False
        except socket.timeout:
            print("Timeout waiting for response")
            return False
        finally:
            # Close the socket
            print("Closing socket...")
            s.close()
            print("Socket closed")
        
    except ConnectionRefusedError:
        print("Connection refused. Is the Unreal MCP server running?")
        return False
    except Exception as e:
        print(f"Error: {e}")
        return False

def main():
    print("Connection cleanup test for Unreal MCP server")
    print("This test will create multiple connections in sequence to verify that the server properly cleans up connections.")
    
    # Try to connect and send a command multiple times
    for i in range(5):
        print(f"\n=== Test {i+1}/5 ===")
        success = connect_and_send_command()
        
        if not success:
            print(f"Test {i+1} failed")
            return
        
        print(f"Test {i+1} succeeded")
        
        # Wait a bit before the next connection
        if i < 4:  # Don't wait after the last test
            wait_time = 2
            print(f"Waiting {wait_time} seconds before next connection...")
            time.sleep(wait_time)
    
    print("\nAll tests completed successfully!")
    print("The server is properly cleaning up connections.")

if __name__ == "__main__":
    main() 