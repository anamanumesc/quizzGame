#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <stdbool.h>
#include <sqlite3.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT 2908 // Port used for communication
#define nr_intrebari 10

extern int errno;
//================================global variables==============================================

typedef struct thData {
    int idThread; // Thread ID for a client
    int cl;       // Socket descriptor for the client
} thData;

struct intrebare {
    int id;
    char intrebare[256];
    char raspunsuri[4][256];
    int raspunsCorect; 
} intrebare[nr_intrebari];

struct player{
    char name[50];
    int score;
} player[10000];

bool active[10000] = {false}; // to know which players are in the game.
int session_in_progress = false;
bool session_started = false;
time_t session_start_time = -1;
int max_active_users = 0; // how many players joined? (used in loops to save time)

bool clientAnswered[10000] = {true}; 

//bool clientAnswered[nr_intrebari][10000] = {true};
//bool allAnswered;
//pthread_mutex_t clientAnsweredMutex = PTHREAD_MUTEX_INITIALIZER;


//================================function calls=======================================================

static void *treat(void *); // Function executed by each thread
void respond(void *);
void read_from_client(struct thData tdL, char *buffer, size_t bufferSize, const char *errorMsg);
void write_to_client(struct thData tdL, const char *message, const char *errorMsg);
void nickname(void *arg);
void print_all_players_scores(); //for debugging
void create_database();
void incarcaIntrebariDinDB();
void afiseazaIntrebari(); //for debugging
bool quizz_logic(thData *client_data);
void send_question_to_client(struct thData *client_data, int question_index);
int check_answer_and_update_score(struct thData *client_data, int question_index);
bool receive_start_command(thData *client_data);
int compare_players(const void *a, const void *b);
void print_winners();
void send_winners_to_client(struct thData *client_data);
void mark_answer_received(int clientID);
void wait_for_all_answers();
void initialize_clientAnswered();
void print_client_responses_status();
void reset_answer_flags();

//=================================main function =================================================================
int main() {

    //creating the database
    incarcaIntrebariDinDB();
    //afiseazaIntrebari();
    printf("done with the database\n");

    struct sockaddr_in server;
    struct sockaddr_in from;
    pthread_t th[100]; 
    int i = 0;
    int sd; // socket descriptor
//creeaza socket
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("[server]Error at socket().\n");
        return errno;
    }

    int on = 1;
    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    bzero(&server, sizeof(server));
    bzero(&from, sizeof(from));

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(PORT); //portul

//bind
    if (bind(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1) { //p + i
        perror("[server]Error at bind().\n");
        return errno;
    }
//listen
    if (listen(sd, 2) == -1) {
        perror("[server]Error at listen().\n");
        return errno;
    }

    while (1) {
        int client;
        thData *td;
        int length = sizeof(from);

        printf("[server]Waiting at port %d...\n", PORT);
        fflush(stdout);
//accept ... client = nou descriptor de socket

        if ((client = accept(sd, (struct sockaddr *)&from, &length)) < 0) {
            perror("[server]Error at accept().\n");
            continue;
        }

        td = (thData*)malloc(sizeof(thData));
        td->idThread = i++;
        td->cl = client;

        pthread_create(&th[i], NULL, &treat, td); 
    }
}

static void *treat(void *arg) {
    thData tdL = *((thData*)arg); //iei informatiile din pointerul arg
    
    fflush(stdout);
    pthread_detach(pthread_self());
    //threadul devine detasabil deci resursele sunt eliminate automat
  

    nickname(arg);
    printf("done with the nicknames\n");

     if (!receive_start_command(&tdL)) {

        printf("Comanda primita nu este 'start'.\n");
     }else{
        printf("Comanda 'start' primita. Inceperea jocului pentru clientul %d.\n", tdL.idThread);

        if(quizz_logic(&tdL)){    


        printf("finished the game\n");
        printf("printing all scores\n");
        print_all_players_scores();
        printf("printing all the score in order\n\n");

    //print_winners();
    send_winners_to_client(&tdL);


        }
     }

    close((intptr_t)arg);
    return NULL;

}

void nickname(void *arg){

thData tdL = *((thData*)arg); 
char nickname[50];
read_from_client(tdL, nickname, 50, "Couldnt read nickname");
strcpy(player[tdL.idThread].name, nickname);
player[tdL.idThread].score = -1; //we still dont know if the player will hit start so its not active yet
max_active_users++; //used for loops

}

void print_all_players_scores() {
    printf("Listing all players and their scores:\n");
    for (int i = 0; i < max_active_users; i++) {
        printf("Player Name: %s, Score: %d\n", player[i].name, player[i].score);
    }

}

void incarcaIntrebariDinDB() {
    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc;

    rc = sqlite3_open("questions.db", &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "eroare la deschidere database %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return;
    }

    char *sql = "SELECT Id, Intrebare, Rasp1, Rasp2, Rasp3, Rasp4, RaspunsCorect FROM bazadate;";
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);

    int index = 0;
    
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW && index < nr_intrebari) {
        intrebare[index].id = sqlite3_column_int(stmt, 0);
        strncpy(intrebare[index].intrebare, (char *)sqlite3_column_text(stmt, 1), 255);
        for (int j = 0; j < 4; j++) {
            strncpy(intrebare[index].raspunsuri[j], (char *)sqlite3_column_text(stmt, 2 + j), 255);
        }
        intrebare[index].raspunsCorect = sqlite3_column_int(stmt, 6);
        index++;
    }
    //trebuie sa inchid si db dupa de adaugatttt (nu uita)
    sqlite3_finalize(stmt);
    sqlite3_close(db);
}


bool receive_start_command(thData *client_data) {
    char command[100];
    time_t current_time;

    // Citirea comenzii de la client
    read_from_client(*client_data, command, sizeof(command), "[server] Eroare la citirea comenzii de la client.\n");

    // daca comanda este start
    if (strcmp(command, "start") == 0) {
        // setej nucatorul ca activ
        player[client_data->idThread].score = 0;
        active[client_data->idThread] = true;

        //timpul curent
        current_time = time(NULL);

        // e primul start?
        if (session_start_time == -1) {
            session_start_time = current_time;
        }

        // au trecut 20 sec de la primul start?
        if (current_time - session_start_time > 20) {
            session_in_progress = true;
            write_to_client(*client_data, "Sesiunea de joc deja a inceput.\n", "[server] Sesiunea de joc deja a inceput.");
            active[client_data->idThread] = false;
            //todo: clientul sa nu mai fie activ
            pthread_exit(NULL); // inchide thread
            return false;
        } else {
            // daca nu se incalca timpul incepem
            write_to_client(*client_data, "Incepem jocul!\n", "[server] Incepem jocul!");
             return true;
        }

  
    }
    else if (strcmp(command, "quit") == 0) {

        write_to_client(*client_data, "S-a executat comanda quit.\n", "eroare la comanda quit la trimitere catre server.");
        pthread_exit(NULL); 
          return false;
     }

    return false;
}

void afiseazaIntrebari() { //debugging
    for (int i = 0; i < nr_intrebari; i++) {
        printf("ID: %d\n", intrebare[i].id);
        printf("Intrebare: %s\n", intrebare[i].intrebare);
        printf("Raspunsuri:\n");
        for (int j = 0; j < 4; j++) {
            printf("  %d. %s\n", j + 1, intrebare[i].raspunsuri[j]);
        }
        printf("Raspuns corect: %d\n\n", intrebare[i].raspunsCorect);
    }
}

bool quizz_logic(thData *client_data) {

        char clientNotification[50];
        initialize_clientAnswered(); //setam cu true
   
    for (int i = 0; i < nr_intrebari; i++) {
        printf("\n!!!!!!!!!!!!!!!!!!!!!!!!!\nincepe WHILE LOOP IN server PT INTREBAREA %i\n inainte de a incepe, aici avem flagurile\n", i+1);
        print_client_responses_status();

         // Wait for client notification
        read_from_client(*client_data, clientNotification, sizeof(clientNotification), "[server] Error reading notification from client.");
        
        // Check if the notification is correct
        char expectedNotification[50];
        snprintf(expectedNotification, sizeof(expectedNotification), "Ready for question %d", i + 1);

        if (strstr(clientNotification, expectedNotification) == NULL) {
            printf("[server] Unexpected notification received: %s\n", clientNotification);
            break; // Break out of the loop if the notification is not as expected
        }

        send_question_to_client(client_data, i);

        //functia chech answer and update score imi returneaza daca nu am primit inca raspunsul
        int result = check_answer_and_update_score(client_data, i);
        if (result == 1) {
            write_to_client(*client_data, "Raspuns corect!\n", "[server] Eroare la trimiterea confirmarii raspunsului corect.");
        } else if (result == 2) {
            write_to_client(*client_data, "Raspuns gresit!\n", "[server] Eroare la trimiterea confirmarii raspunsului gresit.");
        } else if (result == 3) {
            write_to_client(*client_data, "Timpul a expirat! Raspunsul nu este luat in considerare.\n", "[server] Eroare la trimiterea mesajului de timp expirat.");
        } else if (result == 4) {
            write_to_client(*client_data, "La revedere...\n", "[server] Eroare la trimiterea mesajului de quit.");
            pthread_exit(NULL); //edit FLAG
            return false;
        }
        printf("\n BEFORE WAIT FOR ALL CLIENTD\n");
        print_client_responses_status();


        wait_for_all_answers(); //asteptam toate raspunsurile.. daca nu s au primit toate raspunsurile aceasta functie sta pe loc
        //are efect blocant

        printf("\n AFTER WAIT FUNCTION EXITED\n");
        print_client_responses_status();

        sleep(3);
        //toate raspunsurile au fost primite
        reset_answer_flags(); 
        printf("\n am resetat flagul\n trecem la urmatoarea intrebare \n");
        print_client_responses_status();
    }
    return true;


}


void send_question_to_client(struct thData *client_data, int question_index) {
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), "Intrebare: %s\n1. %s\n2. %s\n3. %s\n4. %s\n",
             intrebare[question_index].intrebare,
             intrebare[question_index].raspunsuri[0],
             intrebare[question_index].raspunsuri[1],
             intrebare[question_index].raspunsuri[2],
             intrebare[question_index].raspunsuri[3]);

    write_to_client(*client_data, buffer, "[server] Eroare la trimiterea intrebarii catre client.\n");
}


int check_answer_and_update_score(thData *client_data, int question_index) {
    char client_response[256];
    fd_set set;
    struct timeval timeout;

    // timp asteptare = 10 sec
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;    

    //initializez descriptotii de fisiere cu 0
    FD_ZERO(&set);
    FD_SET(client_data->cl, &set);

    // astept un raspuns de la un client cu select
    int rv = select(client_data->cl + 1, &set, NULL, NULL, &timeout);

    if (rv == -1) {
        perror("Select error");
        return -1; // Eroare la select
    }

    // citesc raspunsul chiar daca timpul nu a expirat
    read_from_client(*client_data, client_response, sizeof(client_response), "[server] Eroare la citirea raspunsului clientului.\n");

    if (strcmp(client_response, "quit") == 0) {
        active[client_data->idThread] = false;
        mark_answer_received(client_data->idThread);
        return 4; // Comanda quit
    }

    if (rv == 0) {
        // daca timpul a expirat
        mark_answer_received(client_data->idThread);
        return 3; // 3 = timp expirat
    }

    int client_answer = atoi(client_response);
    if (client_answer == intrebare[question_index].raspunsCorect) {
        player[client_data->idThread].score++; // Actualizare scor
        mark_answer_received(client_data->idThread);
        return 1; // 1 = rasp corect
    } else {
        mark_answer_received(client_data->idThread);
        return 2; // rasp gresit
    }
}

void send_winners_to_client(struct thData *client_data) {
    // buffer clasament
    char winners_message[10000];
    int offset = 0; 

    // fac un struct 
    struct player active_players[max_active_users];
    int active_count = 0;

    // jucatori activi
    for (int i = 0; i < max_active_users; i++) {
        if (active[i]) {
            active_players[active_count++] = player[i];
        }
    }

    // sortare jucatori dupa scor
    qsort(active_players, active_count, sizeof(struct player), compare_players);
    printf("nu a inceput inca creerea mesajului\n");
    // constuiesc clasament jucatori
    offset += snprintf(winners_message + offset, sizeof(winners_message) - offset, "Clasament jucatori activi:\n");
    
    printf("nu a inceput inca creerea mesajului 2\n");
    for (int i = 0; i < active_count; i++) {
        offset += snprintf(winners_message + offset, sizeof(winners_message) - offset, "%s: %d puncte\n", active_players[i].name, active_players[i].score);
        printf("am printat un jucator\n");
    
    }

    printf("am printat toti jucatorii activi, acum trebuie sa scriem clintului\n");

    // trimit clasament
    write_to_client(*client_data, winners_message, "Eroare la trimiterea clasamentului către client.\n"); 
       
    printf("the clasament has been sent to client!\n");
}


int compare_players(const void *a, const void *b) {
    struct player *playerA = (struct player *)a;
    struct player *playerB = (struct player *)b;

    // sortez dupa scor
    return playerB->score - playerA->score;
}

//-----------------------------------------------------------
void read_from_client(struct thData tdL, char *buffer, size_t bufferSize, const char *errorMsg) {
    memset(buffer, 0, bufferSize); // Clear the buffer

    ssize_t readBytes = read(tdL.cl, buffer, bufferSize - 1); // Leave space for null terminator
    if (readBytes == -1) {
        // Error occurred
        fprintf(stderr, "[server][Thread %d] %s: ", tdL.idThread, errorMsg);
        perror("");
    } else {
        buffer[readBytes] = '\0'; // Ensure null termination
        printf("[server][Thread %d] Received message from client: %s\n", tdL.idThread, buffer);
    }
}

void write_to_client(struct thData tdL, const char *message, const char *errorMsg) {
    if (write(tdL.cl, message, strlen(message) + 1) == -1) {
        perror(errorMsg);
    }else {
 printf("[server][Thread %d] Sent message to client: %s\n", tdL.idThread, message);
    }
}


void wait_for_all_answers() {
    int allAnswered = false;

    while (!allAnswered) {
        allAnswered = true; 

        //pthread_mutex_lock(&clientAnsweredMutex);

        for (int i = 0; i < max_active_users; i++) {
            if (active[i] && !clientAnswered[i]) {
                allAnswered = false; //an active player didnt respond = false
                break; 
            }
        }
        //pthread_mutex_unlock(&clientAnsweredMutex); 

        if (!allAnswered) {
            sleep(1); // asteptam si verificam din nou
        }
    }
}


void mark_answer_received(int clientID) {
    clientAnswered[clientID] = true;
}

void reset_answer_flags() {
    for (int i = 0; i < max_active_users; i++) {
        clientAnswered[i] = false;
    }

    //allAnswered = false;

}

void initialize_clientAnswered() {
   // pthread_mutex_lock(&clientAnsweredMutex);
    for (int i = 0; i < max_active_users; i++) {
        if (active[i]) {
            clientAnswered[i] = false;
        }
    }
    //allAnswered = false;

    //pthread_mutex_unlock(&clientAnsweredMutex);
}

void print_client_responses_status() {
    //pthread_mutex_lock(&clientAnsweredMutex);

    printf("Statusul răspunsurilor clienților:\n");
    for (int i = 0; i < max_active_users; i++) {
        if (active[i]) {
            printf("Client %d (%s) - %s\n", i, player[i].name, clientAnswered[i] ? "A raspuns" : "Nu a raspuns inca");
        }
    }

    //pthread_mutex_unlock(&clientAnsweredMutex);
}
