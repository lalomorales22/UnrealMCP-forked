#!/usr/bin/env python
import socket
import json
import time
import sys
import argparse

def connect_to_server(host="localhost", port=9876, timeout=5, delay_before_connect=0):
    """Connect to the server with optional delay and timeout."""
    if delay_before_connect > 0:
        print(f"Waiting {delay_before_connect} seconds before connecting...")
        time.sleep(delay_before_connect)
    
    try:
        # Create socket
        print(f"\n=== Connecting to {host}:{port} ===")
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(timeout)
        
        # Connect to server
        print(f"Attempting connection to {host}:{port}...")
        s.connect((host, port))
        print("Connected successfully")
        return s
    except ConnectionRefusedError:
        print("Connection refused. Is the server running?")
        return None
    except Exception as e:
        print(f"Connection error: {e}")
        return None

def send_command(sock, command_type="get_scene_info", params=None):
    """Send a command to the server and return the response."""
    if not sock:
        print("No socket connection available")
        return False
    
    if params is None:
        params = {}
    
    command = {
        "type": command_type,
        "params": params
    }
    
    try:
        print(f"Sending command: {json.dumps(command)}")
        sock.sendall(json.dumps(command).encode('utf-8'))
        print("Command sent")
        
        # Wait for response
        print("Waiting for response...")
        response = sock.recv(8192)
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
    except Exception as e:
        print(f"Error during command: {e}")
        return False

def close_connection(sock):
    """Close the socket connection."""
    if sock:
        try:
            print("Closing socket...")
            sock.close()
            print("Socket closed")
        except Exception as e:
            print(f"Error closing socket: {e}")

def run_single_test(host="localhost", port=9876, delay=0):
    """Run a single connection test."""
    sock = connect_to_server(host, port, delay_before_connect=delay)
    if not sock:
        return False
    
    success = send_command(sock)
    close_connection(sock)
    return success

def run_multiple_tests(count=5, delay_between=5, host="localhost", port=9876):
    """Run multiple connection tests with delay between them."""
    print(f"Running {count} connection tests with {delay_between}s delay between tests")
    
    successes = 0
    for i in range(count):
        print(f"\n=== Test {i+1}/{count} ===")
        if run_single_test(host, port, delay=0 if i == 0 else delay_between):
            print(f"Test {i+1} succeeded")
            successes += 1
        else:
            print(f"Test {i+1} failed")
        
        # Always add a delay after a test, even if it failed
        if i < count - 1:  # Don't wait after the last test
            print(f"Waiting {delay_between} seconds before next test...")
            time.sleep(delay_between)
    
    print(f"\nTests completed: {successes}/{count} successful")
    return successes == count

def main():
    parser = argparse.ArgumentParser(description="Test connection to Unreal MCP server")
    parser.add_argument("--host", default="localhost", help="Server hostname")
    parser.add_argument("--port", type=int, default=9876, help="Server port")
    parser.add_argument("--count", type=int, default=3, help="Number of connection tests to run")
    parser.add_argument("--delay", type=float, default=5.0, help="Delay between connection tests (seconds)")
    args = parser.parse_args()
    
    print("=== Unreal MCP Server Connection Test ===")
    print(f"Host: {args.host}")
    print(f"Port: {args.port}")
    print(f"Tests: {args.count}")
    print(f"Delay: {args.delay}s")
    
    success = run_multiple_tests(
        count=args.count,
        delay_between=args.delay,
        host=args.host,
        port=args.port
    )
    
    if success:
        print("\nAll tests completed successfully!")
        sys.exit(0)
    else:
        print("\nSome tests failed. Check the logs for details.")
        sys.exit(1)

if __name__ == "__main__":
    main() 