#!/usr/bin/env python
import socket
import json
import time
import sys
import traceback

def main():
    print("Debug connection test for Unreal MCP server")
    print(f"Python version: {sys.version}")
    print(f"Platform: {sys.platform}")
    
    try:
        # Create socket
        print("\n=== Creating socket ===")
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        print(f"Socket created: {s}")
        print(f"Socket family: {s.family}")
        print(f"Socket type: {s.type}")
        print(f"Socket proto: {s.proto}")
        
        # Set timeout
        timeout = 10
        print(f"\n=== Setting timeout to {timeout} seconds ===")
        s.settimeout(timeout)
        print(f"Socket timeout: {s.gettimeout()}")
        
        # Connect to server
        server_address = ("localhost", 9876)
        print(f"\n=== Connecting to {server_address} ===")
        try:
            s.connect(server_address)
            print("Connected successfully")
            print(f"Socket peer name: {s.getpeername()}")
            print(f"Socket sock name: {s.getsockname()}")
        except Exception as e:
            print(f"Connection failed: {e}")
            traceback.print_exc()
            s.close()
            return
        
        # Send a simple command
        command = {
            "type": "get_scene_info",
            "params": {}
        }
        
        command_json = json.dumps(command)
        command_bytes = command_json.encode('utf-8')
        print(f"\n=== Sending command ({len(command_bytes)} bytes) ===")
        print(f"Command: {command_json}")
        
        try:
            bytes_sent = s.sendall(command_bytes)
            print(f"Command sent successfully")
        except Exception as e:
            print(f"Send failed: {e}")
            traceback.print_exc()
            s.close()
            return
        
        # Wait for response
        print("\n=== Waiting for response ===")
        buffer_size = 8192
        print(f"Buffer size: {buffer_size} bytes")
        
        try:
            start_time = time.time()
            response = s.recv(buffer_size)
            end_time = time.time()
            elapsed = end_time - start_time
            
            print(f"Received {len(response)} bytes in {elapsed:.3f} seconds")
            
            if response:
                try:
                    response_str = response.decode('utf-8')
                    print(f"Raw response: {response_str}")
                    
                    response_data = json.loads(response_str)
                    print(f"Parsed JSON response: {json.dumps(response_data, indent=2)}")
                except UnicodeDecodeError as e:
                    print(f"Unicode decode error: {e}")
                    print(f"Raw bytes: {response}")
                except json.JSONDecodeError as e:
                    print(f"JSON decode error: {e}")
                    print(f"Response string: {response_str}")
            else:
                print("No response received (connection closed by server)")
        except socket.timeout:
            print(f"Timeout waiting for response after {timeout} seconds")
        except Exception as e:
            print(f"Receive failed: {e}")
            traceback.print_exc()
        
        # Close the socket
        print("\n=== Closing socket ===")
        try:
            s.close()
            print("Socket closed successfully")
        except Exception as e:
            print(f"Close failed: {e}")
            traceback.print_exc()
        
    except Exception as e:
        print(f"Unexpected error: {e}")
        traceback.print_exc()

if __name__ == "__main__":
    main() 