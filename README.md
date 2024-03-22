## Multi-Player Quiz Game using TCP Sockets (C Language)

This is the project for my Computer Networks class . It implements a real-time multiplayer quiz experience using TCP sockets in C language.

## Configuring the Game

Download the following files: 
1. server.c 
2. client.c 
3. questions.db

Compile the Application:
* Server: ```gcc server.c -o server -lsqlite3 -lpthread```
* client: ```gcc client.c -o client```

## How to Play

1. Open two terminal windows.
2. In the first terminal, run the server:
```./server```
3. In the second terminal, run the client, specifying the server IP and port (default 2908):
```./client 127.0.0.1 2908```
4. Repeat step 3 for additional clients to join the game.

## Rules

* A single player initiates the quiz by typing the "start" command.
* Other players have 20 seconds to join.
* The quiz consists of 10 questions, each with a 20-second time limit for answering.
* The server ensures all players finish answering a question before advancing to the next one.
* Players can exit the game at any point (before or during a question) by typing "quit".
