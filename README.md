# Multi-threaded Game
Simple terminal based game that uses IPC (Inter Process Communication) mechanisms to synchronise player's movement.
## Files
* server.c: Main server program, there can only be one instance of it running.
* client.c: Client program, there can by multiple instances running at the same time.
* conn.h: Encapsulates all IPC-specific mechanisms that are responsible for client-server initial connection.
* session.h: Encapsulates all IPC-specific mechanisms that are responsible for real time client-server connection.
* structs.h: Contains all game data, such as map and rules.
* utils.h: Common ncurses functionality and basic interrupt implementation. It also contains caught keyboard signals.
## Prerequisites
* UNIX-based operating system with POSIX threads. 
* Nucurses library installed.
## Building the Sample
To build the game's clients using the command prompt:
```
gcc server.c -o server -lncursesw -lpthread -lrt
gcc client.c -o client -lncursesw -lpthread -lrt
gcc bot.c -o bot -lncursesw -lpthread -lrt
```
## Game screenshot
![Server's view](screenshot.png?raw=true "")
