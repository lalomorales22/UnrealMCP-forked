#!/usr/bin/env python
import socket
import time
import sys

def main():
    print("Basic connection test for Unreal MCP server")
    
    try:
        # Create socket
        print("Creating socket...")
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(5)  # 5 second timeout
        
        # Connect to server
        print("Connecting to localhost:9876...")
        s.connect(("localhost", 9876))
        print("Connected successfully")
        
        # Just wait for a bit
        print("Waiting for 3 seconds...")
        time.sleep(3)
        
        # Close the socket
        print("Closing socket...")
        s.close()
        print("Socket closed")
        
    except ConnectionRefusedError:
        print("Connection refused. Is the Unreal MCP server running?")
    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    main() 