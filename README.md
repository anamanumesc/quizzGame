##Multi-Player Quiz Game using TCP Sockets##
my final project for the Computer Networks class, second year
written in C language

##Configuring the game##
1. Download server.c, client.c, and questions.db
2. Compile the server with ```gcc server.c -o server -lsqlite3 -lpthread ```
3. Compile the client with ```gcc client.c -o client```


##How to play##
1. In one terminal window, ```./server```
2. In a different terminal window, ```./client 127.0.0.1 2908```
3. Repeat step 2 for multiple clients

##Rules##
-One player initiates the quiz with the "start" command. Other players have 20 seconds to join in
-There are 10 questions, each question has 20 a seconds time limit to answer.
-The server waits for everyone to finish before moving to the next question. 
-You can type "quit" at any point, whether before the quiz starts or during a question, to exit the game
