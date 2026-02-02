#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#define BUFFER_SIZE 128


void display_help() {
 
    printf("Format: cmd type start length [whence]\n");
    printf("cmd: 'g' (F_GETLK), 's' (F_SETLK), 'w' (F_SETLKW), '?' to display this help\n");
    printf("type: 'r' (F_RDLCK), 'w' (F_WRLCK) or 'u' (F_UNLCK)\n");
    printf("start: lock starting offset\n");
    printf("length: number of bytes to lock\n");
    printf("whence: 's' (SEEK_SET, default), 'c' (SEEK_CUR) or 'e'(SEEK_END)\n");
}


int main(int argc, char *argv[]) {
   
    if (argc != 2) {
        fprintf(stderr, "error: %s requires exactly one file argument\n", argv[0]);
    }

    // ouvre le fichier en écriture / lecture
    int file_descriptor = open(argv[1], O_RDWR);
    if (file_descriptor < 0) {
        perror("Error opening file");
        return -1;
    }

    
    while (1) {
        char command_buffer[BUFFER_SIZE], command, lock_type_char;
        int start, length;
        char whence;

    
        printf("Enter ? for help\nPID=%d> ", getpid());

        // lit entrée standard et stocke dans buffer
        fgets(command_buffer, BUFFER_SIZE, stdin);
        // les espaces sont traités comme des délimiteurs dans scanf pour parser
        sscanf(command_buffer, "%c %c %d %d %c", &command, &lock_type_char, &start, &length, &whence);

        if (command == '?') {
            display_help();
            continue;
        }

        // Configurer une structure lock
        struct flock fl;
        int fcntl_command = (command == 'g') ? F_GETLK : (command == 's') ? F_SETLK : (command == 'w') ? F_SETLKW : - 1 ;
        fl.l_type = (lock_type_char == 'r') ? F_RDLCK : (lock_type_char == 'w') ? F_WRLCK : (lock_type_char == 'u') ? F_UNLCK : -1;
        fl.l_start = start;
        fl.l_len = length;
        fl.l_whence = (whence == 'c') ? SEEK_CUR : (whence == 'e') ? SEEK_END : SEEK_SET;
        



         if (fcntl_command == -1) {
            fprintf(stderr, "Invalid command: %c\n", command);
            continue;
        }
       


     // fnctl va changer les champs de fl, notamment le type, selon la 1ere commande 
        if (fcntl(file_descriptor, fcntl_command, &fl) == -1) {
            perror("Cannot place a lock at the moment");
            continue;
        }

        // le champ type est changé par fnctl
        if (command == 'g') {
            if (fl.l_type == F_UNLCK) {
                printf("[PID=%d] No lock on start: %d of length: %d\n", getpid(), start, length);
            } else {
                printf("[PID=%d] There is a: %s lock on start: %d of length: %d on whence : %d detained by %d\n", 
                       getpid(), (fl.l_type == F_RDLCK) ? "read" : "write", start, length, fl.l_whence, fl.l_pid);
            }
        } 
        
        else if (command == 's' || command == 'w') {
         if (lock_type_char == 'u') { printf("[PID=%d] Unlocking the region starting at: %d of length: %d\n", getpid(), start, length);} 
                
        else {printf("[PID=%d] Putting a: %s lock on start: %d of length: %d\n", getpid(), (fl.l_type == F_RDLCK) ? "read" : "write", start, length);}
    
    }

}
    close(file_descriptor);
    return 0;



}