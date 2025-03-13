# MCP Server Test Scripts

This directory contains test scripts for the Unreal MCP Server.

## Overview

These scripts test various aspects of the MCP Server functionality:

1. **Basic Connection Test** (`1_basic_connection.py`): Tests the basic connection to the MCP Server.
2. **Python Execution Test** (`2_python_execution.py`): Tests executing Python code through the MCP Server.
3. **String Handling Test** (`3_string_test.py`): Tests various string formats and potential problem areas.

## Running the Tests

You can run individual tests:

```bash
python 1_basic_connection.py
python 2_python_execution.py
python 3_string_test.py
```

Or run all tests in sequence:

```bash
python run_all_tests.py
```

## Test Requirements

- The MCP Server must be running in Unreal Engine
- Python 3.6 or higher
- Socket and JSON modules (included in standard library)

## Command Format

The MCP Server expects commands in the following format:

```json
{
  "type": "command_name",
  "code": "python_code_here"  // For execute_python command
}
```

The command should be sent as a JSON string followed by a newline character.

## Troubleshooting

If you encounter issues:

1. Make sure the MCP Server is running in Unreal Engine
2. Check that you're connecting to the correct host and port (default: localhost:13377)
3. Verify the command format is correct
4. Check the Unreal Engine log for any error messages

## Adding New Tests

When adding new tests, follow the pattern of the existing tests:

1. Connect to the server
2. Send a command
3. Receive and process the response
4. Return success/failure

Use the `sys.exit()` code to indicate test success (0) or failure (non-zero). 