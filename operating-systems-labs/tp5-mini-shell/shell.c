#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

pid_t fg_pid = 0;   


pid_t bg_pid = 0;  


char** parser(char *input, int *bg_flag) {
    
    int size = 5;
    char **tokens = malloc(size * sizeof(char*));
    // retourne un pointeur sur le premier element de l entree standard
    char *token = strtok(input, " ");
    
    int i = 0;
    while (token) {
        tokens[i++] = token;
        if (i >= size) {
            size += 5;
            tokens = realloc(tokens, size * sizeof(char*));
        }
        // passe au prochain element de l entree standard. strtok enregistre l endroit ou il s est arreter entre  chaque appel de fonctions
        token = strtok(NULL, " ");
    }
    tokens[i] = NULL;
    
    if (strcmp(tokens[i-1], "&") == 0) {
        tokens[i-1] = NULL;
        *bg_flag = 1;
    } else {
        *bg_flag = 0;
    }
    
    return tokens;
}


// crtl c 
void signal_sigint(int signum) {
    if (fg_pid > 0) {
     kill(fg_pid, SIGINT);
// le shell(processus parent ici) attend que le processus en foreground se termine effectivement, et reprend 
     waitpid(fg_pid, NULL, 0);
    }
}

// ferme le terminal et tue tous les processus enfants associé
void signal_sighup(int signum) { 

    // sighup est dabord envoyé vers TOUS les processus/jobs fils du shell(ici on en aura que 2, fg et bg) avant de fermer le processus courant aka le shell
    if (fg_pid > 0) kill(fg_pid, SIGHUP);
    if (bg_pid > 0) kill(bg_pid, SIGHUP);
    exit(0);
}


// le SE/OS envoie dans tout les cas sigchld au parent quand un p fils meurt, le waitpid ici est pour eviter que le p en bg soit un zombie

void signal_sigchld(int signum, siginfo_t *info, void *context) {
    if (info->si_pid == bg_pid) {
        waitpid(bg_pid, NULL, 0);
        bg_pid = 0;
        printf("\nBackground job finished\n>meinShell:~%s$ ", getcwd(NULL, 0));  // printf ecrit sur la sortie standard
        fflush(stdout); // force l affichage du texte en sortie standard
    }
}



void signal_handlers() {
    struct sigaction act1, act2;

    // crtl+c
    act1.sa_handler = signal_sigint;
    act1.sa_flags = SA_RESTART; // gere lappel syst waitpid qui est bloquant (typiquement psq les signaux le debloque)
    sigaction(SIGINT, &act1, NULL);

   // EXIT
    act1.sa_handler = signal_sighup;
    act1.sa_flags = 0;
    sigaction(SIGHUP, &act1, NULL);

   // signaux a ignorer
    act1.sa_handler = SIG_IGN;
    act1.sa_flags = 0;
    sigaction(SIGTERM, &act1, NULL);
    sigaction(SIGQUIT, &act1, NULL);

    
    act2.sa_sigaction = signal_sigchld;
    act2.sa_flags = SA_SIGINFO | SA_RESTART;
    sigaction(SIGCHLD, &act2, NULL);
}








int main() {
   
    signal_handlers(); 

    while (1) {
        char input[100];
        int bg_flag = 0;

// getcwd parametre d allouement, taille
        printf(">meinShell:~%s$ ", getcwd(NULL, 0));


        if (fgets(input, sizeof(input), stdin) != NULL) {

// remplace la fin de lentree standard par \0 car \0 marque la fin dune chaine
            size_t end  = strcspn(input, "\n");
            input[end] = '\0';
            
            char **argv = parser(input, &bg_flag);

  // retour a while
            if (argv[0] == NULL) {
                continue;
            }

            // commandes builtin 

            if (strcmp(argv[0], "exit") == 0) {
                printf("Exiting the Shell \n");
                free(argv);
                raise(SIGHUP);
            }

             else if (strcmp(argv[0], "cd") == 0) {
                const char *dir = argv[1];
                // getenv recupere la variable denvironment de home
                 if (dir == NULL) { dir = getenv("HOME");} 
                 //change de repertoire 
                if (chdir(dir) != 0) {perror("failed");}

            } 
            
            else {
                    if (bg_flag && bg_pid != 0) {
                         printf(" One background job is allowed to run \n");

          } else {
                pid_t pid = fork();
                if (pid == 0) { 
                    
                    if (bg_flag) {
                        int fd = open("/dev/null", O_RDWR);
                        dup2(fd, 0); // redirect the standard input and output of the background process to /dev/null typiquement pr empecher que ca interfere avec e/s du processus fg
                       dup2(fd, 1);
                        close(fd);

                        struct sigaction act;
                        act.sa_handler = SIG_IGN;
                        sigaction(SIGINT, &act, NULL);
                    }

                    if (execvp(argv[0], argv) == -1) {
                        perror(argv[0]);
                        exit(EXIT_FAILURE);
                    }
                    // si on est dans le p parent, alors on passe le pid du fils a bg ou fg
                } else if (pid > 0) { 
                    if (bg_flag) {
                        bg_pid = pid;
                    } else {
                        fg_pid = pid;
                        // vu que le p courant, soit le shell, est le parent, il attend que le p en fg finisse
                        waitpid(pid, NULL, 0);
                        fg_pid = 0;
                    }
             
                }
            }
            }
            free(argv);
        } 
        
        else {

            continue;
        } // passe a la prochaine commande en cas d erreur de lecture de lentree standard
    }
    return 0;
}

