#!/usr/bin/env python3
"""
MCP Server Test Runner

This script runs all the MCP Server test scripts in sequence.
"""

import subprocess
import sys
import os
import time

def run_test(test_script):
    """Run a test script and return whether it passed."""
    script_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), test_script)
    
    print(f"\n{'=' * 60}")
    print(f"Running {test_script}...")
    print(f"{'=' * 60}")
    
    try:
        result = subprocess.run(
            [sys.executable, script_path],
            capture_output=False,  # Show output in real-time
            check=False
        )
        
        if result.returncode == 0:
            print(f"\n✓ {test_script} PASSED")
            return True
        else:
            print(f"\n✗ {test_script} FAILED (exit code: {result.returncode})")
            return False
    except Exception as e:
        print(f"\n✗ Error running {test_script}: {e}")
        return False

def main():
    """Run all test scripts."""
    # List of test scripts to run
    test_scripts = [
        "1_basic_connection.py",
        "2_python_execution.py",
        "3_string_test.py"
    ]
    
    # Track results
    results = {}
    
    # Run each test
    for script in test_scripts:
        results[script] = run_test(script)
        # Add a small delay between tests
        time.sleep(1)
    
    # Print summary
    print("\n" + "=" * 60)
    print("TEST SUMMARY")
    print("=" * 60)
    
    all_passed = True
    for script, passed in results.items():
        status = "✓ PASSED" if passed else "✗ FAILED"
        print(f"{script}: {status}")
        if not passed:
            all_passed = False
    
    print("\n" + "=" * 60)
    if all_passed:
        print("✓ ALL TESTS PASSED")
        return 0
    else:
        print("✗ SOME TESTS FAILED")
        return 1

if __name__ == "__main__":
    print("=== MCP Server Test Runner ===")
    exit_code = main()
    sys.exit(exit_code) 