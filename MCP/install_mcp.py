#!/usr/bin/env python
"""
This script installs the MCP package and verifies it's installed correctly.
Run this script with the same Python interpreter that Claude Desktop will use.
"""

import sys
import subprocess
import importlib.util

def check_mcp_installed():
    """Check if the MCP package is installed."""
    spec = importlib.util.find_spec("mcp")
    return spec is not None

def install_mcp():
    """Install the MCP package."""
    print(f"Installing MCP package using Python: {sys.executable}")
    try:
        subprocess.check_call([sys.executable, "-m", "pip", "install", "mcp>=0.1.0"])
        print("MCP package installed successfully!")
        return True
    except subprocess.CalledProcessError as e:
        print(f"Error installing MCP package: {e}")
        return False

def main():
    """Main function."""
    print(f"Python version: {sys.version}")
    print(f"Python executable: {sys.executable}")
    
    if check_mcp_installed():
        print("MCP package is already installed.")
        try:
            import mcp
            print(f"MCP version: {mcp.__version__}")
        except (ImportError, AttributeError):
            print("MCP package is installed but version could not be determined.")
    else:
        print("MCP package is not installed.")
        if install_mcp():
            if check_mcp_installed():
                print("Verification: MCP package is now installed.")
                try:
                    import mcp
                    print(f"MCP version: {mcp.__version__}")
                except (ImportError, AttributeError):
                    print("MCP package is installed but version could not be determined.")
            else:
                print("ERROR: MCP package installation failed verification.")
                print("Please try installing manually with:")
                print(f"{sys.executable} -m pip install mcp>=0.1.0")
        else:
            print("Installation failed. Please try installing manually with:")
            print(f"{sys.executable} -m pip install mcp>=0.1.0")
    
    print("\nTo verify the MCP package is installed in the correct environment:")
    print("1. Make sure you run this script with the same Python interpreter that Claude Desktop will use")
    print("2. Check that the Python executable path shown above matches the one in your Claude Desktop configuration")
    
    input("\nPress Enter to exit...") 