"""Utility functions for MCP commands."""

import json
import socket
import sys

# Constants (these will be read from MCPConstants.h)
DEFAULT_PORT = 13377
DEFAULT_BUFFER_SIZE = 65536
DEFAULT_TIMEOUT = 10

try:
    # Try to read the port from the C++ constants
    import os
    plugin_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
    constants_path = os.path.join(plugin_dir, "Source", "UnrealArchitect", "Public", "MCPConstants.h")
    
    if os.path.exists(constants_path):
        with open(constants_path, 'r') as f:
            constants_content = f.read()
            
            # Extract port from MCPConstants
            port_match = constants_content.find("DEFAULT_PORT = ")
            if port_match != -1:
                port_line = constants_content[port_match:].split(';')[0]
                DEFAULT_PORT = int(port_line.split('=')[1].strip())
                
            # Extract buffer size from MCPConstants
            buffer_match = constants_content.find("DEFAULT_RECEIVE_BUFFER_SIZE = ")
            if buffer_match != -1:
                buffer_line = constants_content[buffer_match:].split(';')[0]
                DEFAULT_BUFFER_SIZE = int(buffer_line.split('=')[1].strip())
except Exception as e:
    print(f"Warning: Could not read constants from MCPConstants.h: {e}", file=sys.stderr)

def send_command(command_type, params=None):
    """Send a command to the C++ MCP server and return the response."""
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.settimeout(DEFAULT_TIMEOUT)
            s.connect(("localhost", DEFAULT_PORT))
            command = {
                "type": command_type,
                "params": params or {}
            }
            s.sendall(json.dumps(command).encode('utf-8'))
            
            chunks = []
            response_data = b''
            
            while True:
                try:
                    chunk = s.recv(DEFAULT_BUFFER_SIZE)
                    if not chunk:
                        break
                    chunks.append(chunk)
                    
                    response_data = b''.join(chunks)
                    try:
                        json.loads(response_data.decode('utf-8'))
                        break
                    except json.JSONDecodeError:
                        continue
                except socket.timeout:
                    if response_data:
                        break
                    raise
            
            if not response_data:
                raise Exception("No data received from server")
                
            return json.loads(response_data.decode('utf-8'))
    except ConnectionRefusedError:
        print(f"Error: Could not connect to Unreal MCP server on localhost:{DEFAULT_PORT}.", file=sys.stderr)
        print("Make sure your Unreal Engine with MCP plugin is running.", file=sys.stderr)
        raise Exception("Failed to connect to Unreal MCP server: Connection refused")
    except socket.timeout:
        print("Error: Connection timed out while communicating with Unreal MCP server.", file=sys.stderr)
        raise Exception("Failed to communicate with Unreal MCP server: Connection timed out")
    except Exception as e:
        print(f"Error communicating with Unreal MCP server: {str(e)}", file=sys.stderr)
        raise Exception(f"Failed to communicate with Unreal MCP server: {str(e)}") 