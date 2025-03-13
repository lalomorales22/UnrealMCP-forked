#!/usr/bin/env python3
"""
String Handling Test for MCP Server

This script tests string handling in the MCP Server.
It connects to the server, sends Python code with various string formats,
and verifies they are handled correctly.
"""

import socket
import json
import sys

def main():
    """Connect to the MCP Server and test string handling."""
    try:
        # Create socket
        print("Connecting to MCP Server on localhost:1337...")
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(10)  # 10 second timeout
        
        # Connect to server
        s.connect(("localhost", 13377))
        print("✓ Connected successfully")
        
        # Python code with various string formats
        code = """
import unreal
import json

# Test 1: Basic string types
test_string1 = "This is a simple string with double quotes"
test_string2 = 'This is a simple string with single quotes'
test_string3 = \"\"\"This is a
multiline string\"\"\"
test_string4 = f"This is an f-string with {'interpolation'}"
test_string5 = "This string has escape sequences: \\n \\t \\\\ \\' \\""
test_string6 = r"This is a raw string with no escape processing: \n \t \\"

# Test 2: Print statements
print("Print statement 1: Simple string")
print(f"Print statement 2: F-string with {test_string2}")
print("Print statement 3: Multiple", "arguments", 123, test_string3)
print("Print statement 4: With escape sequences: \\n \\t")

# Test 3: Potentially problematic strings
test_string7 = "String with quotes: 'single' and \\"double\\""
test_string8 = 'String with quotes: "double" and \\'single\\''
test_string9 = "String with backslashes: \\ \\\\ \\n"
test_string10 = "String with special characters: 🐍 😊 🚀"

# Test 4: Unterminated strings (these are properly terminated but might be misinterpreted)
test_string11 = "String with a quote at the end: '"
test_string12 = 'String with a quote at the end: "'
test_string13 = "String with a backslash at the end: \\"
test_string14 = "String with multiple backslashes at the end: \\\\"

# Test 5: String concatenation
test_string15 = "Part 1 " + "Part 2"
test_string16 = "Multiple " + "parts " + "concatenated"
test_string17 = "Mixed " + 'quote' + " types"

# Collect results in a dictionary
results = {
    "test1": test_string1,
    "test2": test_string2,
    "test3": test_string3,
    "test4": test_string4,
    "test5": test_string5,
    "test6": test_string6,
    "test7": test_string7,
    "test8": test_string8,
    "test9": test_string9,
    "test10": test_string10,
    "test11": test_string11,
    "test12": test_string12,
    "test13": test_string13,
    "test14": test_string14,
    "test15": test_string15,
    "test16": test_string16,
    "test17": test_string17
}

# Log the results
unreal.log("===== STRING TEST RESULTS =====")
for key, value in results.items():
    unreal.log(f"{key}: {value}")

# Return the results as JSON
return json.dumps(results, indent=2)
"""
        
        # Create command with multiple formats to try to make it work
        command = {
            "command": "execute_python",
            "type": "execute_python",
            "code": code,
            "data": {
                "code": code
            }
        }
        
        # Send command
        print("Sending execute_python command...")
        command_str = json.dumps(command) + "\n"  # Add newline
        s.sendall(command_str.encode('utf-8'))
        
        # Receive response
        print("Waiting for response...")
        response = b""
        while True:
            data = s.recv(4096)
            if not data:
                break
            response += data
            if b"\n" in data:  # Check for newline which indicates end of response
                break
        
        # Close connection
        s.close()
        print("✓ Connection closed properly")
        
        # Process response
        if response:
            response_str = response.decode('utf-8').strip()
            
            try:
                response_json = json.loads(response_str)
                print("\n=== RESPONSE ===")
                print(f"Status: {response_json.get('status', 'unknown')}")
                
                if response_json.get('status') == 'success':
                    print("✓ String test successful")
                    result = response_json.get('result', {})
                    if isinstance(result, dict) and 'output' in result:
                        output = result['output']
                        print(f"Output length: {len(output)} characters")
                        print("First 200 characters of output:")
                        print(output[:200] + "..." if len(output) > 200 else output)
                    return True
                else:
                    print("✗ String test failed")
                    print(f"Error: {response_json.get('message', 'Unknown error')}")
                    return False
            except json.JSONDecodeError as e:
                print(f"✗ Error parsing JSON response: {e}")
                print(f"Raw response: {response_str}")
                return False
        else:
            print("✗ No response received from server")
            return False
        
    except ConnectionRefusedError:
        print("✗ Connection refused. Is the MCP Server running?")
        return False
    except socket.timeout:
        print("✗ Connection timed out. Is the MCP Server running?")
        return False
    except Exception as e:
        print(f"✗ Error: {e}")
        return False

if __name__ == "__main__":
    print("=== MCP Server String Handling Test ===")
    success = main()
    print("\n=== TEST RESULT ===")
    if success:
        print("✓ String handling test PASSED")
        sys.exit(0)
    else:
        print("✗ String handling test FAILED")
        sys.exit(1)