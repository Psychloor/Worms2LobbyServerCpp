# WormsServer

A modern C++23 implementation of a Worms 2® game server, 
inspired by and partially ported from [Syroot's Worms2 GameServer](https://gitlab.com/Syroot/Worms/-/tree/master/src/tool/Syroot.Worms.Worms2.GameServer).

## Overview

This project implements a game server for Worms 2® that handles multiplayer game sessions, 
room management, and player interactions. 
Built with modern C++ features and focusing on performance and reliability.

## Features

- Modern C++23 implementation
- Asynchronous networking using Boost.Asio
- Multithreaded architecture
- Session management for game, rooms, and players
- Windows-1251 character encoding support
- Comprehensive logging system using spdlog
- Command-line configuration options

## Prerequisites

- C++23 compatible compiler (Clang 16+ or MSVC 2022+)
- CMake 3.31 or higher
- vcpkg package manager
- Git (for cloning repository and submodules)

### Required Dependencies
The following dependencies will be automatically installed through vcpkg:
- Boost 1.84.0 or higher (system, endian)
- spdlog 1.12.0 or higher

## Building

1. Clone the repository with submodules:
```
bash
git clone --recursive https://github.com/yourusername/WormsServer.git
cd WormsServer
```
2. Make sure vcpkg is properly set up:
```
bash
# Set VCPKG_ROOT environment variable (if not already set)
# Windows (PowerShell):
$env:VCPKG_ROOT="C:\path\to\vcpkg"
# Linux/macOS:
export VCPKG_ROOT=/path/to/vcpkg
```
3. Build the project:
```
bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```
### Windows-Specific Setup
If building on Windows, you might need to enable long paths:
```
powershell
# Run as Administrator
Set-ItemProperty -Path "HKLM:\SYSTEM\CurrentControlSet\Control\FileSystem" -Name "LongPathsEnabled" -Value 1
```
## Usage

Run the server with default settings:
```
bash
./worms_server
```
Available command-line options:
- `-p, --port <port>`: Port to listen on (default: 17000)
- `-c, --connections <count>`: Maximum number of connections (default: 10000)
- `-t, --threads <count>`: Maximum number of threads (default: number of CPU cores)
- `-h, --help`: Print the help message

## Configuration

The server can be configured through command-line arguments and supports runtime log level adjustment through environment variables.

### Environment Variables
- `SPDLOG_LEVEL`: Set the logging level (trace, debug, info, warn, error, critical)
Example:
```
bash
# Windows (PowerShell):
$env:SPDLOG_LEVEL="debug"
# Linux/macOS:
export SPDLOG_LEVEL=debug
```
## Logging

Logs are written to both console and daily rotating files:
- Console output with color coding
- Daily log files stored in `logs/worms_server.log`
- Automatic log rotation
- Configurable log levels through environment variables

## Architecture

The server is built using a multithreaded architecture with the following key components:
- Asynchronous network handling using Boost.Asio
- Session management for users and rooms
- Packet processing with custom protocol
- Room and game state management