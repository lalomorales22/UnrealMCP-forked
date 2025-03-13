import json
import socket
import sys

try:
    from mcp.server.fastmcp import FastMCP, Context
except ImportError:
    print("Error: The 'mcp' package is not installed.", file=sys.stderr)
    print("Please install it using: pip install mcp", file=sys.stderr)
    sys.exit(1)

# Initialize the MCP server
mcp = FastMCP(
    "UnrealMCP",
    description="Unreal Engine integration through the Model Context Protocol"
)

def send_command(command_type, params=None):
    """Send a command to the C++ MCP server and return the response."""
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.settimeout(10)  # Set a 10-second timeout
            s.connect(("localhost", 1337))  # Connect to Unreal C++ server
            command = {
                "type": command_type,
                "params": params or {}
            }
            s.sendall(json.dumps(command).encode('utf-8'))
            
            # Read response with a buffer
            chunks = []
            response_data = b''
            
            # Wait for data with timeout
            while True:
                try:
                    chunk = s.recv(8192)
                    if not chunk:  # Connection closed
                        break
                    chunks.append(chunk)
                    
                    # Try to parse what we have so far
                    response_data = b''.join(chunks)
                    try:
                        # If we can parse it as JSON, we have a complete response
                        json.loads(response_data.decode('utf-8'))
                        break
                    except json.JSONDecodeError:
                        # Incomplete JSON, continue receiving
                        continue
                except socket.timeout:
                    # If we have some data but timed out, try to use what we have
                    if response_data:
                        break
                    raise
            
            if not response_data:
                raise Exception("No data received from server")
                
            return json.loads(response_data.decode('utf-8'))
    except ConnectionRefusedError:
        print("Error: Could not connect to Unreal MCP server on localhost:1337.", file=sys.stderr)
        print("Make sure your Unreal Engine with MCP plugin is running.", file=sys.stderr)
        raise Exception("Failed to connect to Unreal MCP server: Connection refused")
    except socket.timeout:
        print("Error: Connection timed out while communicating with Unreal MCP server.", file=sys.stderr)
        raise Exception("Failed to communicate with Unreal MCP server: Connection timed out")
    except Exception as e:
        print(f"Error communicating with Unreal MCP server: {str(e)}", file=sys.stderr)
        raise Exception(f"Failed to communicate with Unreal MCP server: {str(e)}")

@mcp.tool()
def get_scene_info(ctx: Context) -> str:
    """Get detailed information about the current Unreal scene."""
    try:
        response = send_command("get_scene_info")
        if response["status"] == "success":
            return json.dumps(response["result"], indent=2)
        else:
            return f"Error: {response['message']}"
    except Exception as e:
        return f"Error getting scene info: {str(e)}"

@mcp.tool()
def create_object(ctx: Context, type: str, location: list = None, label: str = None) -> str:
    """Create a new object in the Unreal scene."""
    try:
        params = {"type": type}
        if location:
            params["location"] = location
        if label:
            params["label"] = label
        response = send_command("create_object", params)
        if response["status"] == "success":
            return f"Created object: {response['result']['name']} with label: {response['result']['label']}"
        else:
            return f"Error: {response['message']}"
    except Exception as e:
        return f"Error creating object: {str(e)}"

@mcp.tool()
def modify_object(ctx: Context, name: str, location: list = None, rotation: list = None, scale: list = None) -> str:
    """Modify an existing object in the Unreal scene."""
    try:
        params = {"name": name}
        if location:
            params["location"] = location
        if rotation:
            params["rotation"] = rotation
        if scale:
            params["scale"] = scale
        response = send_command("modify_object", params)
        if response["status"] == "success":
            return f"Modified object: {response['result']['name']}"
        else:
            return f"Error: {response['message']}"
    except Exception as e:
        return f"Error modifying object: {str(e)}"

@mcp.tool()
def delete_object(ctx: Context, name: str) -> str:
    """Delete an object from the Unreal scene."""
    try:
        response = send_command("delete_object", {"name": name})
        if response["status"] == "success":
            return f"Deleted object: {name}"
        else:
            return f"Error: {response['message']}"
    except Exception as e:
        return f"Error deleting object: {str(e)}"

@mcp.tool()
def execute_python(ctx: Context, code: str = None, file: str = None) -> str:
    """Execute Python code or a Python script file in Unreal Engine.
    
    Args:
        code: Python code to execute as a string
        file: Path to a Python script file to execute
        
    Note: You must provide either code or file, but not both.
    """
    try:
        if not code and not file:
            return "Error: You must provide either 'code' or 'file' parameter"
        
        if code and file:
            return "Error: You can only provide either 'code' or 'file', not both"
        
        params = {}
        if code:
            params["code"] = code
        if file:
            params["file"] = file
            
        response = send_command("execute_python", params)
        if response["status"] == "success":
            return f"Python execution successful:\n{response['result']['output']}"
        else:
            return f"Error: {response['message']}"
    except Exception as e:
        return f"Error executing Python: {str(e)}"

if __name__ == "__main__":
    print("Starting Unreal MCP server...", file=sys.stderr)
    try:
        mcp.run()  # Start the MCP server
    except Exception as e:
        print(f"Error starting MCP server: {str(e)}", file=sys.stderr)
        sys.exit(1) 