#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

void setup_address(struct sockaddr_in *addr, const char *ip, int port);
int setup_socket(const char *ip, int port);
int receiveInt(int socket_fd);
void sendInt(int socket_fd, int value);

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <Server IP> <Port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int socket_fd = setup_socket(argv[1], atoi(argv[2])); // atoi string to chara

    int attempts = receiveInt(socket_fd);
    printf("Number of attempts: %d.\n", attempts);
    int lowerRange = receiveInt(socket_fd);
    int upperRange = receiveInt(socket_fd);
    printf("Range: %d to %d.\n", lowerRange, upperRange);

    while (1) {
        int guess, response;
        printf("Enter a number in the range: ");
        scanf("%d", &guess); // met l entier en passé en entrée dans guess
        sendInt(socket_fd, guess);
        response = receiveInt(socket_fd);
        
        if (response == 0) { printf("You won!\n"); break; }
        else if (response == 2) { printf("You lost!\n"); break; }
        else if (response == 1) printf("Choose a smaller number.\n");
        else if (response == -1) printf("Choose a larger number.\n");
    }

    close(socket_fd);
    return 0;
}

void setup_address(struct sockaddr_in *addr, const char *ip, int port) {
    memset(addr, 0, sizeof(*addr)); // met addr à 0
    addr->sin_family = AF_INET; // famille dadresse avec laquelle notre socket va pouvoir communiquer. AF_INET pr ipv4
    inet_pton(AF_INET, ip, &(addr->sin_addr)); // converti une chaine de cara en format binaire 
    addr->sin_port = htons(port); // converti le port en un format valide
}

int setup_socket(const char *ip, int port) {


    int socket_fd = socket(AF_INET, SOCK_STREAM, 0); // sock stream cest le type de communication ici tcp, 0 pr le protocol(par defaut tcp) 
    if (socket_fd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    setup_address(&server_addr, ip, port);


    if (connect(socket_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) { // on fait un casting ici psq le 2eme para. de connect est de type sockaddr 
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    return socket_fd;
}

int receiveInt(int socket_fd) {
    int value;
    if (read(socket_fd, &value, sizeof(int)) < 0) {
        perror("Error reading from socket");
        exit(EXIT_FAILURE);
    }
    return value;
}

void sendInt(int socket_fd, int value) {
    if (write(socket_fd, &value, sizeof(int)) < 0) {
        perror("Error writing to socket");
        exit(EXIT_FAILURE);
    }
}
