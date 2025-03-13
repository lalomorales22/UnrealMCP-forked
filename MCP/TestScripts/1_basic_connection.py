#!/usr/bin/env python3
"""
Basic Connection Test for MCP Server

This script tests the basic connection to the MCP Server.
It connects to the server, sends a simple ping command, and verifies the response.
"""

import socket
import json
import sys
import os
import platform
import subprocess
import time
import traceback
from datetime import datetime

def check_port_in_use(host, port):
    """Check if the specified port is already in use."""
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        try:
            s.bind((host, port))
            return False  # Port is available
        except socket.error:
            return True  # Port is in use

def check_service_running(port):
    """Check if any service is running on the specified port using system commands."""
    try:
        if platform.system() == "Windows":
            output = subprocess.check_output(f"netstat -ano | findstr :{port}", shell=True).decode()
            if output:
                return True, output.strip()
        else:
            output = subprocess.check_output(f"lsof -i :{port}", shell=True).decode()
            if output:
                return True, output.strip()
    except subprocess.CalledProcessError:
        pass  # Command returned error or no output
    except Exception as e:
        print(f"Error checking service: {e}")
    
    return False, "No service detected"

def log_system_info():
    """Log system information to help with debugging."""
    print("\n=== SYSTEM INFORMATION ===")
    print(f"Operating System: {platform.system()} {platform.version()}")
    print(f"Python Version: {platform.python_version()}")
    print(f"Machine: {platform.machine()}")
    print(f"Node: {platform.node()}")
    print(f"Time: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    
    try:
        # Check firewall status on Windows
        if platform.system() == "Windows":
            firewall_output = subprocess.check_output("netsh advfirewall show currentprofile", shell=True).decode()
            if "State                                 ON" in firewall_output:
                print("Windows Firewall: ENABLED")
            else:
                print("Windows Firewall: DISABLED")
    except Exception as e:
        print(f"Error checking firewall: {e}")

def ping_host(host, timeout=2):
    """Ping the host to check basic connectivity."""
    try:
        if platform.system() == "Windows":
            ping_cmd = f"ping -n 1 -w {int(timeout*1000)} {host}"
        else:
            ping_cmd = f"ping -c 1 -W {int(timeout)} {host}"
        
        result = subprocess.run(ping_cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        if result.returncode == 0:
            return True, "Host is reachable"
        else:
            return False, f"Host unreachable: {result.stdout.decode().strip()}"
    except Exception as e:
        return False, f"Error pinging host: {str(e)}"

def trace_socket_errors(error):
    """Get detailed information about a socket error."""
    error_info = {
        10035: "WSAEWOULDBLOCK: Resource temporarily unavailable, operation would block",
        10036: "WSAEINPROGRESS: Operation now in progress",
        10037: "WSAEALREADY: Operation already in progress",
        10038: "WSAENOTSOCK: Socket operation on non-socket",
        10039: "WSAEDESTADDRREQ: Destination address required",
        10040: "WSAEMSGSIZE: Message too long",
        10041: "WSAEPROTOTYPE: Protocol wrong type for socket",
        10042: "WSAENOPROTOOPT: Bad protocol option",
        10043: "WSAEPROTONOSUPPORT: Protocol not supported",
        10044: "WSAESOCKTNOSUPPORT: Socket type not supported",
        10045: "WSAEOPNOTSUPP: Operation not supported",
        10046: "WSAEPFNOSUPPORT: Protocol family not supported",
        10047: "WSAEAFNOSUPPORT: Address family not supported by protocol",
        10048: "WSAEADDRINUSE: Address already in use",
        10049: "WSAEADDRNOTAVAIL: Cannot assign requested address",
        10050: "WSAENETDOWN: Network is down",
        10051: "WSAENETUNREACH: Network is unreachable",
        10052: "WSAENETRESET: Network dropped connection on reset",
        10053: "WSAECONNABORTED: Software caused connection abort",
        10054: "WSAECONNRESET: Connection reset by peer",
        10055: "WSAENOBUFS: No buffer space available",
        10056: "WSAEISCONN: Socket is already connected",
        10057: "WSAENOTCONN: Socket is not connected",
        10058: "WSAESHUTDOWN: Cannot send after socket shutdown",
        10059: "WSAETOOMANYREFS: Too many references",
        10060: "WSAETIMEDOUT: Connection timed out",
        10061: "WSAECONNREFUSED: Connection refused",
        10062: "WSAELOOP: Cannot translate name",
        10063: "WSAENAMETOOLONG: Name too long",
        10064: "WSAEHOSTDOWN: Host is down",
        10065: "WSAEHOSTUNREACH: No route to host",
    }
    
    if hasattr(error, 'errno'):
        errno = error.errno
        description = error_info.get(errno, f"Unknown error code: {errno}")
        return f"Socket error {errno}: {description}"
    return f"Unknown socket error: {str(error)}"

def main():
    """Connect to the MCP Server and verify the connection works."""
    host = "localhost"
    port = 13377
    timeout = 5  # 5 second timeout
    
    print("\n=== NETWORK DIAGNOSTICS ===")
    # Check if we can ping the host
    ping_success, ping_msg = ping_host(host)
    print(f"Ping test: {ping_msg}")
    
    # Check if port is already in use locally
    port_in_use = check_port_in_use("127.0.0.1", port)
    if port_in_use:
        print(f"Warning: Port {port} is already in use on this machine")
    else:
        print(f"Port {port} is not in use on this machine")
    
    # Check if any service is running on the target port
    service_running, service_details = check_service_running(port)
    if service_running:
        print(f"A service is running on port {port}:")
        print(service_details)
    else:
        print(f"No service detected on port {port}")
    
    try:
        print("\n=== CONNECTION TEST ===")
        print(f"Creating socket to connect to {host}:{port}...")
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(timeout)
        
        # Try to connect
        print(f"Attempting connection to {host}:{port}...")
        connect_start = time.time()
        s.connect((host, port))
        connect_time = time.time() - connect_start
        print(f"✓ Connected successfully in {connect_time:.2f} seconds")
        
        # Create a simple get_scene_info command
        command = {
            "type": "get_scene_info"
        }
        
        # Send command
        print("Sending get_scene_info command...")
        command_str = json.dumps(command) + "\n"  # Add newline
        s.sendall(command_str.encode('utf-8'))
        
        # Receive response with timeout tracking
        print("Waiting for response...")
        response_start = time.time()
        response = b""
        while True:
            try:
                data = s.recv(4096)
                if not data:
                    print("Connection closed by server")
                    break
                
                response += data
                print(f"Received {len(data)} bytes")
                
                if b"\n" in data:  # Check for newline which indicates end of response
                    print("Received newline character, end of response")
                    break
                
                # Check if we've been waiting too long
                if time.time() - response_start > timeout:
                    print(f"Response timeout after {timeout} seconds")
                    break
            except socket.timeout:
                print(f"Socket timeout after {timeout} seconds")
                break
        
        response_time = time.time() - response_start
        print(f"Response received in {response_time:.2f} seconds")
        
        # Close connection
        try:
            s.shutdown(socket.SHUT_RDWR)
        except Exception as e:
            print(f"Warning during socket shutdown: {str(e)}")
            
        s.close()
        print("✓ Connection closed properly")
        
        # Process response
        if response:
            response_str = response.decode('utf-8').strip()
            print(f"Raw response ({len(response_str)} bytes): {response_str[:100]}...")
            
            try:
                response_json = json.loads(response_str)
                print("\n=== RESPONSE ===")
                print(f"Status: {response_json.get('status', 'unknown')}")
                
                if response_json.get('status') == 'success':
                    print("✓ Server responded successfully")
                    print(f"Level: {response_json.get('result', {}).get('level', 'unknown')}")
                    print(f"Actor count: {response_json.get('result', {}).get('actor_count', 0)}")
                    return True
                else:
                    print("✗ Server responded with an error")
                    print(f"Error: {response_json.get('message', 'Unknown error')}")
                    return False
            except json.JSONDecodeError as e:
                print(f"✗ Error parsing JSON response: {e}")
                print(f"Raw response: {response_str}")
                return False
        else:
            print("✗ No response received from server")
            return False
        
    except ConnectionRefusedError as e:
        print(f"✗ Connection refused: {trace_socket_errors(e)}")
        print("This typically means:")
        print("  1. The MCP Server is not running")
        print("  2. The server is running on a different port")
        print("  3. A firewall is blocking the connection")
        return False
    except socket.timeout as e:
        print(f"✗ Connection timed out: {trace_socket_errors(e)}")
        print("This typically means:")
        print("  1. The MCP Server is running but not responding")
        print("  2. A firewall or security software is intercepting but not blocking the connection")
        print("  3. The network configuration is preventing the connection")
        return False
    except Exception as e:
        print(f"✗ Error: {trace_socket_errors(e)}")
        print("Detailed error information:")
        traceback.print_exc()
        return False

if __name__ == "__main__":
    print("=== MCP Server Basic Connection Test ===")
    print(f"Time: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    
    # Log system information
    log_system_info()
    
    # Run the main test
    success = main()
    
    print("\n=== TEST RESULT ===")
    if success:
        print("✓ Connection test PASSED")
        sys.exit(0)
    else:
        print("✗ Connection test FAILED")
        sys.exit(1) 