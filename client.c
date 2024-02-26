/* cliTCPIt.c - TCP client example
   The client sends multiple string messages to the server and receives responses.

   Author: Lenuta Alboaie <adria@info.uaic.ro> (c)
*/
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <stdbool.h>
//==========================function calls=================================
void send_to_server(int *sd, const char *message, const char *errorMsg);
void read_from_server(int *sd, char *buffer, size_t bufferSize, const char *errorMsg);
void welcome_message(int *sd);
void quizz(int *sd); 
bool start_as_input(int *sd);
void receive_and_print_ranking(int *sd);
void notify_server_before_question(int *sd, int questionNumber);
//===========================global variables===============================
extern int errno; // Error code returned by some calls
int port;
char client_name[50];
int nr_intrebari = 10;
//=========================main function=====================================

int main(int argc, char *argv[]) {
    int sd; // Socket descriptor
    struct sockaddr_in server;
    char msg[100]; // Message buffer

    if (argc != 3) {
        printf("Syntax: %s <server_address> <port>\n", argv[0]);
        return -1;
    }

    port = atoi(argv[2]);

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Error at socket().\n");
        return errno;
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(argv[1]);
    server.sin_port = htons(port);

    if (connect(sd, (struct sockaddr *) &server, sizeof(struct sockaddr)) == -1) {
        perror("[client]Error at connect().\n");
        return errno;
    }


        welcome_message(&sd);

    if (start_as_input(&sd)) {
        quizz(&sd);
        receive_and_print_ranking(&sd);
    } else {
        printf("Încheierea execuției clientului.\n");
    }

    close(sd);
}

//================functions definition=====================

void welcome_message(int *sd){


        printf("                     Welcome to the QuizGame! \n Enter your name: ");

        fflush (stdout);
        if (fgets(client_name, sizeof(client_name), stdin) == NULL) {
            perror("[client]Eroare la citirea NUMELUI de la tastatura.\n");
        }
        // elimin caracterul newline din sirul de caractere
        client_name[strcspn(client_name, "\n")] = 0;
    
        printf("                     Hi, %s!\n-You will be prompted n questions\n-And you have 10 seconds to answer them \n ", client_name);
        printf("If you dont answer in time you will get 0 points \n To exit the game type quit \n");
    
        send_to_server(sd, client_name, "Error sending name to server");

}

bool start_as_input(int *sd) {
    char command[100];
    char serverResponse[100];

    printf("To start the game type start: ");
    fflush(stdout);

    // Citirea comenzii de la tastatură
    if (fgets(command, sizeof(command), stdin) == NULL) {
        perror("[client] Eroare la citirea comenzii de la tastatura.\n");
        return false;
    }

    // Eliminarea caracterului newline
    command[strcspn(command, "\n")] = 0;

    // Trimiterea comenzii către server
    send_to_server(sd, command, "[client] Eroare la trimiterea comenzii la server.\n");

    // Verifică dacă comanda este "start"
    if (strcmp(command, "start") == 0) {
        // Citirea răspunsului serverului
            read_from_server(sd, serverResponse, sizeof(serverResponse), "[client] Eroare la citirea răspunsului de la server.\n");

            // Verifică dacă sesiunea de joc este deja în desfășurare
            if (strcmp(serverResponse, "Sesiunea de joc deja a inceput.\n") == 0) {
                printf("%s", serverResponse);
                return false; // Încheie execuția clientului
            }
            printf("s-a citti start ies din functie\n");
            return true; // Continuă execuția
    }
    //daca comanda este quit
    else if (strcmp(command, "quit") == 0) {
    read_from_server(sd, serverResponse, sizeof(serverResponse), "[client] Eroare la citirea răspunsului de la server.\n");
    printf("%s", serverResponse);
                return false; // Încheie execuția clientului

    }

    return false;
}


void quizz(int *sd) {
    char question[1024];
    char response[100];
    char serverMessage[5000]; // buffer raspuns

    for (int questionNumber = 1; questionNumber <= nr_intrebari; questionNumber++) {

        printf("Next is: question nr %d. \n\n", questionNumber);
        notify_server_before_question(sd, questionNumber);

        read_from_server(sd, question, 1024, "[client]Eroare la citirea intrebarii de la server.\n");
        printf("Am citit de la server \n\n");
        
        // mesajul e o intrebare valida
        if (strlen(question) == 0) {
            printf("Mesaj invalid primit!\n");
            break;
        }

        // afisare raspuns + cerere raspuns
        printf("\n%s\nYour answer: ", question);
        fflush(stdout);
        bzero(response, 100);

        if (fgets(response, 100, stdin) == NULL) {
            perror("[client]Eroare la citirea raspunsului de la tastatura\n");
            break;
        }

        // elimin newline din raspuns
        response[strcspn(response, "\n")] = 0;

        // trimit rapsunsul la server     // todo: pot sa trimit si comanda quit!!!!
        if (write(*sd, response, strlen(response) + 1) <= 0) {
            perror("[client]Eroare la trimiterea raspunsului la server\n");
            break;
        }

        // primirea daca raspunsul e corect sau nu
        //aici primesc si mesaj ca clientul a renuntat
        read_from_server(sd, serverMessage, 5000, "[client]Eroare la citirea raspunsului serverului.\n");

        printf("%s\n", serverMessage); // Aafisez mesaj de la server gresit/corect/quit
        

        if (strcmp(serverMessage, "Goodbye...\n") == 0) {
            printf("Game over.\n");
            close(*sd); 
            exit(0);
        }

        fflush(stdin);
    }

    printf("Ai ajuns la finalul intrebarilor!\n");
}



void notify_server_before_question(int *sd, int questionNumber) {
    char notification[50];
    snprintf(notification, sizeof(notification), "Ready for question %d", questionNumber);

    if (write(*sd, notification, strlen(notification) + 1) <= 0) {
        perror("[client] Error sending notification to server.");
    }
}


void receive_and_print_ranking(int *sd) {
    char ranking[10000]; // buffer clasament
    printf("astept rankingul sa il citesc de la server\n");
    //citesc de la server
    read_from_server(sd, ranking, sizeof(ranking), "[client]Eroare la citirea clasamentului de la server.\n");

    //afisez clasament
    printf("%s\n", ranking);
}


void send_to_server(int *sd, const char *message, const char *errorMsg) {
    if (write(*sd, message, strlen(message) + 1) == -1) {
        perror(errorMsg);
    }
}

void read_from_server(int *sd, char *buffer, size_t bufferSize, const char *errorMsg) {
    bzero(buffer, bufferSize); // Curat buffer

    ssize_t readBytes = read(*sd, buffer, bufferSize - 1); 
    if (readBytes == -1) {
        // A apărut o eroare
        perror(errorMsg);
    } else {
        buffer[readBytes] = '\0'; // null terminare
        //printf("[client]Received message from server: %s\n", buffer);
    }
}
