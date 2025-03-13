import json
import socket
import sys

def test_connection():
    """Test the connection to the Unreal Engine C++ MCP server."""
    print("Testing connection to Unreal Engine MCP server on localhost:9876...")
    
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.settimeout(5)  # Set a timeout of 5 seconds
            s.connect(("localhost", 9876))
            print("✓ Successfully connected to the server!")
            
            # Try to send a simple command
            command = {
                "type": "get_scene_info",
                "params": {}
            }
            print("Sending 'get_scene_info' command...")
            s.sendall(json.dumps(command).encode('utf-8'))
            
            # Try to receive a response
            response_data = s.recv(8192)
            response = json.loads(response_data.decode('utf-8'))
            print("✓ Received response from server!")
            print("\nResponse:")
            print(json.dumps(response, indent=2))
            
            return True
    except ConnectionRefusedError:
        print("✗ Connection refused. Is the Unreal Engine with MCP plugin running?")
        print("  Make sure your Unreal Engine is running and the MCP plugin is enabled.")
        return False
    except socket.timeout:
        print("✗ Connection timed out. The server might be running but not responding.")
        return False
    except json.JSONDecodeError:
        print("✗ Received invalid JSON response from the server.")
        return False
    except Exception as e:
        print(f"✗ Error: {str(e)}")
        return False

if __name__ == "__main__":
    success = test_connection()
    if not success:
        print("\nTroubleshooting tips:")
        print("1. Make sure your Unreal Engine is running")
        print("2. Verify that the MCP plugin is enabled in your Unreal project")
        print("3. Check that the plugin is configured to listen on port 9876")
        print("4. Look for any error messages in the Unreal Engine log")
        sys.exit(1)
    else:
        print("\nConnection test successful! The MCP server should work with Claude Desktop.")
        sys.exit(0) 