Ruleset for Unreal MCP Plugin Codebase
1. Code Structure and Organization
Modular Design: Organize code into distinct modules or classes with single responsibilities. For example:
MCPTCPServer.cpp manages TCP server operations.
MCPCommandHandlers.cpp handles command logic.
UnrealMCP.cpp oversees plugin initialization and UI integration.
Naming Conventions: Adhere to Unreal Engine’s coding standards:
Classes: PascalCase (e.g., FMCPTCPServer).
Functions: camelCase (e.g., ProcessCommand).
Variables: camelCase with prefixes where applicable (e.g., bRunning).
Header Files: Use #pragma once for simplicity. Only include necessary headers to minimize dependencies.
Source Files: Limit each .cpp file to a single class or closely related functions to enhance readability and maintainability.
2. Error Handling and Logging
Comprehensive Error Checking: Validate all function return values, especially for network operations (e.g., socket creation in MCPTCPServer::Start) and Unreal Engine API calls.
Logging: Leverage the existing logging system (e.g., MCP_LOG_INFO, MCP_LOG_ERROR) to track operations and errors. Use appropriate log levels:
INFO for significant events (e.g., server start/stop).
ERROR for failures (e.g., socket errors).
VERBOSE for detailed debugging (e.g., received data).
Exception Handling: Avoid exceptions in C++ code unless critical, as Unreal Engine’s codebase is not exception-safe. Use return codes instead (e.g., bool returns in Start).
3. Security Considerations
Input Validation: Rigorously validate all incoming network data in MCPTCPServer::ProcessCommand and command handlers to prevent injection attacks or crashes from malformed JSON.
Authentication: Consider adding a basic authentication mechanism (e.g., token-based) to restrict command execution to authorized clients, especially since commands can modify the Unreal scene.
Data Sanitization: Sanitize inputs before processing (e.g., ensure ActorName in FMCPDeleteObjectHandler is safe to use).
4. Performance Optimization
Efficient Networking: Optimize MCPTCPServer::ProcessClientData to handle multiple clients without blocking the main thread. Use non-blocking sockets and asynchronous operations where feasible.
Resource Management: Ensure proper cleanup of resources (e.g., sockets in CleanupClientConnection, ticker handles in Stop) to prevent leaks.
Tick Efficiency: Minimize work in Tick (e.g., ProcessPendingConnections) to maintain editor responsiveness.
5. Documentation and Comments
Inline Comments: Explain complex logic, such as the JSON parsing in ProcessCommand or timeout logic in CheckClientTimeouts.
Function Documentation: Use Doxygen-style comments for all public functions, detailing purpose, parameters, and return values. Example:
cpp

Collapse

Wrap

Copy
/**
 * Starts the TCP server on the configured port.
 * @return True if the server started successfully, false otherwise.
 */
bool FMCPTCPServer::Start();
User Documentation: Include a README.md with setup instructions, usage examples, and security notes.
6. Testing and Validation
Unit Tests: Create tests for individual components (e.g., command handlers in MCPCommandHandlers.cpp) using Unreal’s automation testing framework.
Integration Tests: Test the full system with a sample client (e.g., unreal_mcp_server.py) to verify end-to-end functionality.
Stress Testing: Simulate multiple clients and high command loads to ensure stability under pressure.
7. Version Control Practices
Git Repository: Use Git with a branching strategy (e.g., Git Flow) to manage development:
main: Stable releases.
develop: Integration branch.
Feature branches: feature/new-command.
Commit Messages: Write clear, descriptive messages (e.g., “Add timeout handling to MCPTCPServer”).
Code Reviews: Encourage reviews for significant changes to maintain quality.
8. Specific Guidelines for Core Files
MCPTCPServer.cpp:
Ensure concurrent client handling (e.g., non-blocking sockets in ProcessClientData).
Implement timeout mechanisms (already present in CheckClientTimeouts) with configurable durations.
Handle disconnections gracefully (e.g., CleanupClientConnection).
UnrealMCP.cpp:
Guarantee proper plugin lifecycle (e.g., cleanup in ShutdownModule).
Validate UI registration (e.g., avoid duplicate toolbar extensions in ExtendLevelEditorToolbar).
MCPCommandHandlers.cpp:
Isolate each handler for independent testing (e.g., FMCPGetSceneInfoHandler).
Validate all parameters (e.g., type and location in FMCPCreateObjectHandler).
Prevent unsafe operations (e.g., limit execute_python to safe commands).
9. Python Companion Script Guidelines
Environment Setup: Document Python setup (e.g., pip install mcp) in README.md.
Cross-Platform Support: Ensure run_unreal_mcp.bat has alternatives (e.g., .sh for Linux/Mac).
Security: Warn users about exposing the Python server externally and suggest localhost-only operation.
10. Licensing and Contribution
License: Select a license (e.g., MIT) and include a LICENSE file. Check Unreal Engine EULA for compatibility.
Contribution Guidelines: Create a CONTRIBUTING.md with coding standards, pull request process, and issue reporting instructions.