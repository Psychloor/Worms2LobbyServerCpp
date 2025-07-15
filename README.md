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

- C++23 compatible compiler
- CMake 3.25 or higher
- Boost libraries
- spdlog library

## Building
```bash
mkdir build
cd build
cmake ..
cmake --build .
```
## Usage

Run the server with default settings:
```bash
./worms_server
```

Available command-line options:
- `-p, --port <port>`: Port to listen on (default: 17 000)
- `-c, --connections <count>`: Maximum number of connections (default: 10 000)
- `-t, --threads <count>`: Maximum number of threads (default: number of CPU cores)
- `-h, --help`: Print the help message

## Configuration

The server can be configured through command-line arguments and supports runtime log level adjustment through environment variables.

## Logging

Logs are written to both console and daily rotating files:
- Console output with color coding
- Daily log files stored in `logs/worms_server.log`
- Automatic log rotation
- Configurable log levels through environment variables

## Architecture

The server is built using a multithreaded architecture with the following key components:
- Asynchronous network handling
- Session management
- Packet processing
- Room management

## License

MIT License

Copyright © 2025 Psychloor (Kevin Blomqvist)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.


## Acknowledgments

- Original implementation inspiration: [Syroot's Worms2 GameServer](https://gitlab.com/Syroot/Worms/-/tree/master/src/tool/Syroot.Worms.Worms2.GameServer)

## Disclaimer

Worms® is a registered trademark of Team17. This project is not affiliated with or endorsed by Team17.
