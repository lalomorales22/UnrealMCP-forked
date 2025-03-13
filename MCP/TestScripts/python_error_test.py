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
    """Test the execute_python command with code that will generate errors."""
    try:
        # Create socket
        print(f"Connecting to localhost:{DEFAULT_PORT}...")
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(DEFAULT_TIMEOUT)  # timeout
        
        # Connect to server
        s.connect(("localhost", DEFAULT_PORT))
        print("Connected successfully")
        
        # Test executing Python code that will generate an error
        print("\n=== Testing Python code with errors ===")
        code = """
import unreal
import sys

# This will work fine
print("Starting error test...")
print(f"Python version: {sys.version}")

# This will generate a NameError
print(undefined_variable)

# This code will never be reached
print("This line should not be printed")
"""
        
        command = {
            "type": "execute_python",
            "params": {
                "code": code
            }
        }
        
        print(f"Sending execute_python command with code that will generate an error...")
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
                    # Handle the error response format
                    result = response.get('result', {})
                    output = result.get('output', '')
                    error = result.get('error', '')
                    
                    print("\n=== Error Test Results ===")
                    if output:
                        print(f"--- Output ---\n{output}")
                    
                    if error:
                        print(f"--- Error ---\n{error}")
                        
                    # Verify that we got the expected error
                    if "NameError" in error and "undefined_variable" in error:
                        print("\n✅ Test PASSED: Correctly captured the NameError")
                    else:
                        print("\n❌ Test FAILED: Did not capture the expected NameError")
                else:
                    print(f"Error: {response.get('message', 'Unknown error')}")
            except json.JSONDecodeError:
                print(f"Raw response (not valid JSON): {response_data}")
        else:
            print("No response received")
        
        # Close the socket
        s.close()
        print("Socket closed")
        
        # Now test a syntax error
        print("\n=== Testing Python code with syntax error ===")
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(DEFAULT_TIMEOUT)
        s.connect(("localhost", DEFAULT_PORT))
        
        code_with_syntax_error = """
import unreal

# This has a syntax error (missing closing parenthesis
print("This will cause a syntax error"
"""
        
        command = {
            "type": "execute_python",
            "params": {
                "code": code_with_syntax_error
            }
        }
        
        print(f"Sending execute_python command with code that has a syntax error...")
        s.sendall(json.dumps(command).encode('utf-8'))
        
        # Wait for response
        response_data = b''
        while True:
            chunk = s.recv(DEFAULT_BUFFER_SIZE)
            if not chunk:
                break
            response_data += chunk
            try:
                json.loads(response_data.decode('utf-8'))
                break
            except json.JSONDecodeError:
                continue
        
        if response_data:
            try:
                response = json.loads(response_data.decode('utf-8'))
                print(f"Response status: {response.get('status', 'unknown')}")
                
                if response.get('status') == 'error':
                    result = response.get('result', {})
                    output = result.get('output', '')
                    error = result.get('error', '')
                    
                    print("\n=== Syntax Error Test Results ===")
                    if output:
                        print(f"--- Output ---\n{output}")
                    
                    if error:
                        print(f"--- Error ---\n{error}")
                        
                    # Verify that we got the expected error
                    if "SyntaxError" in error:
                        print("\n✅ Test PASSED: Correctly captured the SyntaxError")
                    else:
                        print("\n❌ Test FAILED: Did not capture the expected SyntaxError")
            except json.JSONDecodeError:
                print(f"Raw response (not valid JSON): {response_data}")
        
        s.close()
        
    except ConnectionRefusedError:
        print("Connection refused. Is the server running?")
        return False
    except Exception as e:
        print(f"Error: {e}")
        return False
    
    return True

if __name__ == "__main__":
    print("=== Python Error Handling Test ===")
    if main():
        print("Test completed successfully")
        sys.exit(0)
    else:
        print("Test failed")
        sys.exit(1) 