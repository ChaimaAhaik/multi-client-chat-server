# Multi-Client Chat Server (IRC-like)

Simple multi-client chat server implemented in C++98 using BSD sockets and `poll()`.
It implements a subset of IRC-like behavior: channels, basic modes, private messages, and user registration (PASS/NICK/USER).

## Features
- TCP server using `poll()` for multiplexing
- Support for multiple clients and named channels
- Channel modes: +i (invite-only), +t (topic-operator-only), +k (key/password), +l (user limit), +o (operator)
- Basic command support: `PASS`, `NICK`, `USER`, `JOIN`, `PRIVMSG`, `MODE`, `KICK`, `TOPIC`, `INVITE`, `PART`, `QUIT`
- Bot detection: messages containing certain rude words cause a kick by an internal BOT

## Requirements
- Linux or UNIX-like OS
- Standard C++ compiler with C++98 support (the Makefile uses `c++` with `-std=c++98`)

## Build
Build with the provided `Makefile`:

```sh
make
```

This produces the executable `ircserv` (see `Makefile`). Compiler flags are defined in the `Makefile` (`-std=c++98 -Wall -Wextra -Werror`).

## Run
Usage:

```sh
./ircserv <port> <password>
```

Example:

```sh
./ircserv 6667 secretpass
```

Then connect with `nc` or `telnet` to test manually:

```sh
nc localhost 6667
```

Minimal registration sequence (from client):

```
PASS secretpass

NICK alice

USER alice 0 * :Alice Example
```

After registration you can `JOIN #room`, send `PRIVMSG #room :hello`, etc.

## Configuration
- `my_macros.hpp` contains compile-time limits:
  - `MAX_CLIENTS` (default 10)
  - `BUF_SIZE` (default 512)

## Project layout
- Channel.cpp / Channel.hpp — channel management, modes, broadcasting
- Client.cpp / Client.hpp — per-client state and buffering
- Server.cpp / Server.hpp — main server loop, command parsing and core logic
- main.cpp — argument parsing and server startup
- Makefile — build rules
- my_macros.hpp — small configuration macros

## Notes & Limitations
- The server is a learning / exercise implementation and does not fully implement the RFC IRC protocol.
- There is an internal BOT used for simple moderation/kick on some bad-words detection.
