#!/usr/bin/env python
# format_test.py - Tests different command formats for MCP Server

import socket
import json
import sys
import time

# Configuration
HOST = '127.0.0.1'
PORT = 13377
TIMEOUT = 5  # seconds

# Simple Python code to execute
PYTHON_CODE = """
import unreal
return "Hello from Python!"
"""

def send_command(command_dict):
    """Send a command to the MCP Server and return the response."""
    try:
        # Create a socket connection
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.settimeout(TIMEOUT)
            s.connect((HOST, PORT))
            
            # Convert command to JSON and send
            command_json = json.dumps(command_dict)
            print(f"Sending command format: {command_json}")
            s.sendall(command_json.encode('utf-8'))
            
            # Receive response
            response = b""
            while True:
                chunk = s.recv(4096)
                if not chunk:
                    break
                response += chunk
            
            # Parse and return response
            if response:
                try:
                    return json.loads(response.decode('utf-8'))
                except json.JSONDecodeError:
                    return {"status": "error", "message": "Invalid JSON response", "raw": response.decode('utf-8')}
            else:
                return {"status": "error", "message": "Empty response"}
    except socket.timeout:
        return {"status": "error", "message": "Connection timed out"}
    except ConnectionRefusedError:
        return {"status": "error", "message": "Connection refused. Is the MCP Server running?"}
    except Exception as e:
        return {"status": "error", "message": f"Error: {str(e)}"}

def test_format(format_name, command_dict):
    """Test a specific command format and print the result."""
    print(f"\n=== Testing Format: {format_name} ===")
    response = send_command(command_dict)
    print(f"Response: {json.dumps(response, indent=2)}")
    if response.get("status") == "success":
        print(f"✅ SUCCESS: Format '{format_name}' works!")
        return True
    else:
        print(f"❌ FAILED: Format '{format_name}' does not work.")
        return False

def main():
    print("=== MCP Server Command Format Test ===")
    print(f"Connecting to {HOST}:{PORT}")
    
    # Test different command formats
    formats = [
        ("Format 1: Basic", {
            "command": "execute_python",
            "code": PYTHON_CODE
        }),
        
        ("Format 2: Type field", {
            "type": "execute_python",
            "code": PYTHON_CODE
        }),
        
        ("Format 3: Command in data", {
            "command": "execute_python",
            "data": {
                "code": PYTHON_CODE
            }
        }),
        
        ("Format 4: Type in data", {
            "type": "execute_python",
            "data": {
                "code": PYTHON_CODE
            }
        }),
        
        ("Format 5: Command and params", {
            "command": "execute_python",
            "params": {
                "code": PYTHON_CODE
            }
        }),
        
        ("Format 6: Type and params", {
            "type": "execute_python",
            "params": {
                "code": PYTHON_CODE
            }
        }),
        
        ("Format 7: Command and type", {
            "command": "execute_python",
            "type": "python",
            "code": PYTHON_CODE
        }),
        
        ("Format 8: Command, type, and data", {
            "command": "execute_python",
            "type": "python",
            "data": {
                "code": PYTHON_CODE
            }
        })
    ]
    
    success_count = 0
    for format_name, command_dict in formats:
        if test_format(format_name, command_dict):
            success_count += 1
        time.sleep(1)  # Brief pause between tests
    
    print(f"\n=== Test Summary ===")
    print(f"Tested {len(formats)} command formats")
    print(f"Successful formats: {success_count}")
    print(f"Failed formats: {len(formats) - success_count}")

if __name__ == "__main__":
    main() 