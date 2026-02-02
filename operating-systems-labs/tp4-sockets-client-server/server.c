#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>



void setup_address(struct sockaddr_in *address, int port);
int setup_socket(int port);
int random_number();
void sendInt(int socket_fd, int value);
int receiveInt(int socket_fd);
void interact(int clientSock, struct sockaddr_in clientAddress);

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <Port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int port = atoi(argv[1]);

    int serverSocket = setup_socket(port);

    if (listen(serverSocket, 3) < 0) {
        perror("Failed to listen on socket");
        close(serverSocket); 
        exit(EXIT_FAILURE);
    }

    while (1) {
       
        struct sockaddr_in clientAddress;
        socklen_t clientLength = sizeof(clientAddress);

        int clientSocket = accept(serverSocket, (struct sockaddr *) &clientAddress, &clientLength); 
        // accepte est bloquante, on attend des connection avant de passer a la suite du code
        // elle extrait 1st connexion en queue, et passe ip à clientaddress
        if (clientSocket < 0) {
            perror("Accept failed");
            continue;
        }

        pid_t pid_f = fork();
        if (pid_f == 0) 
        {
            pid_t pid_pf = fork();
            if (pid_pf == 0) { // est adopter par init 
            interact(clientSocket, clientAddress);
            exit(0); // termine le processus 
            }
            else exit(0); // idem
        } 
        
        else if (pid_f > 0) {  waitpid(pid_f, NULL, 0); } // on recupere le fils pour eviter zombie

        else {perror("Fork failed");}
    }

    close(serverSocket);
    return 0;
}

void setup_address(struct sockaddr_in *address, int port) {
    memset(address, 0, sizeof(*address));
    address->sin_family = AF_INET;
    address->sin_addr.s_addr = htonl(INADDR_ANY); // assigne adresse de lhote a ce champ et htonl le converti en big endian
    address->sin_port = htons(port);
}

int setup_socket(int port) {
    struct sockaddr_in address;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }

    setup_address(&address, port);
    if (bind(sock, (struct sockaddr *) &address, sizeof(address)) < 0 ){ //associe la socket a l ip et port qui va ecouter
        perror("Failed to setup socket");
        exit(EXIT_FAILURE);
    }


    return sock;
}

int random_number() {
    int fd = open("/dev/urandom", O_RDONLY);
    int integr;
    if (read(fd, &integr, 1) < 0) { // 3eme argument : taille en octet, 1 octet ici psq le chiffre
        perror("Error reading random number");
        exit(EXIT_FAILURE);
    }
    close(fd);
    return integr;
}

void sendInt(int client_fd, int value) {
    if (write(client_fd, &value, sizeof(int)) < 0) {
        perror("Error writing to client");
    }
}

int receiveInt(int socket_fd) {
    int value;
    if (read(socket_fd, &value, sizeof(int)) < 0) {
        perror("Error reading from socket");
        exit(EXIT_FAILURE);
    }
    return value;
}

void interact(int clientSocket, struct sockaddr_in clientAddress) {
    printf("Client connected: %s\n", inet_ntoa(clientAddress.sin_addr));

    int numAttempts = 3;
    int range[2] = {23, 125};
    sendInt(clientSocket, numAttempts);
    sendInt(clientSocket, range[0]);
    sendInt(clientSocket, range[1]);

    int targetNumber = random_number() % (range[1] - range[0] + 1) + range[0];
    printf("Target number for the client is %d\n", targetNumber);

    for (int i = 0; i < numAttempts - 1; ++i) {
        int guess = receiveInt(clientSocket);

        if (guess == targetNumber) {
            sendInt(clientSocket, 0); 
            printf("Client guessed correctly!\n");
            return; 
        } else if (guess < targetNumber) {
            sendInt(clientSocket, -1); 
        } else {
            sendInt(clientSocket, 1); 
        }
    }


    int finalGuess = receiveInt(clientSocket);
    if (finalGuess == targetNumber) {
        sendInt(clientSocket, 0); 
        printf("Client guessed correctly!\n");
    } else {
        sendInt(clientSocket, 2); 
        sendInt(clientSocket, targetNumber);
        printf("Client did not guess the number. It was %d\n", targetNumber);
    }

    return;
}


