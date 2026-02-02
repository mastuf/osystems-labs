#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>

#define BUFFER_SIZE 1024


void listing(const char *path) {
    struct stat src_stat;

    // stocke le statut du répertoire dans path stat
    // lstat se comporte comme stat en labscence de symlink 
    // on l utilise ici psq on en besoin pr savoir si un fichier est un symlink
    // vu que stat dereferencie les symlink 
    if (lstat(path, &src_stat) != 0) {
        perror("Error obtaining file status");
        return;
    }

    
    char dateStr[30];
    strftime(dateStr, sizeof(dateStr), "%a %b %d %H:%M:%S %Y", localtime(&(src_stat.st_mtime)));



    printf("%c%c%c%c%c%c%c%c%c%c %12lu %s %s\n", 
           S_ISDIR(src_stat.st_mode) ? 'd' : (S_ISLNK(src_stat.st_mode) ? 'l' : '-'), 
           (src_stat.st_mode & S_IRUSR) ? 'r' : '-', 
           (src_stat.st_mode & S_IWUSR) ? 'w' : '-', 
           (src_stat.st_mode & S_IXUSR) ? 'x' : '-',
           (src_stat.st_mode & S_IRGRP) ? 'r' : '-', 
           (src_stat.st_mode & S_IWGRP) ? 'w' : '-', 
           (src_stat.st_mode & S_IXGRP) ? 'x' : '-',
           (src_stat.st_mode & S_IROTH) ? 'r' : '-', 
           (src_stat.st_mode & S_IWOTH) ? 'w' : '-', 
           (src_stat.st_mode & S_IXOTH) ? 'x' : '-',
           src_stat.st_size,
           dateStr, 
           path);

    
    if (S_ISDIR(src_stat.st_mode)) {
        DIR *dir = opendir(path);
        if (!dir) {
            perror("Error opening directory");
            return;
        }

        struct dirent *dp;
        while ((dp = readdir(dir)) != NULL) {
            if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0) {
                char newPath[1000];
                snprintf(newPath, sizeof(newPath), "%s/%s", path, dp->d_name); // snprintf concatène path/dp->d_name et le stocke dans newPath
                listing(newPath);  
            }
        }
        closedir(dir);
    }
}



void processPath(char *srcPath, char *dstPath, int modify_permissions, int follow_symlinks) {
    
    // structure qui va stocker les informations nécessaire des répertoires / fichiers src et destination

    struct stat src_stat, dst_stat;
    
    int src_fd, dst_fd, n;
    char buffer[BUFFER_SIZE];



    if (lstat(srcPath, &src_stat) != 0) {
        perror("Error obtaining file status");
        return;
    }

    
    
    // gestion des répertoires
    if (S_ISDIR(src_stat.st_mode)) {
        if (mkdir(dstPath, 0777) == -1 && errno != EEXIST) {  // créer le répertoire source dans le rep dest 
            perror("Error creating destination directory");
            return;
        }

        DIR *dir = opendir(srcPath);
        if (!dir) {
            perror("Error opening directory");
            return;
        }

        struct dirent *dp;
        while ((dp = readdir(dir)) != NULL) {
            if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0) {
                char newSrcPath[1000], newDstPath[1000];
                snprintf(newSrcPath, sizeof(newSrcPath), "%s/%s", srcPath, dp->d_name); 
                // Src devient source/file.txt
                snprintf(newDstPath, sizeof(newDstPath), "%s/%s", dstPath, dp->d_name); 
                // Dst devient dest/file.txt, dou le "int dst_exists = lstat(dstPath, &dst_stat);"
                processPath(newSrcPath, newDstPath, modify_permissions, follow_symlinks);
            }
        }
        closedir(dir);
    } else {

    
    
    // gère les fichier et liens symboliques
    int dst_exists = lstat(dstPath, &dst_stat);

        if (dst_exists != 0 || src_stat.st_size != dst_stat.st_size || src_stat.st_mtime > dst_stat.st_mtime) {
            // si le fichier nexiste pas ou ect
           

          // pas de copie directe car la struct lstat ne contient pas le path du target 
         
         
          // utilité de lstat est quon peut utiliser s_islnk 
            if (S_ISLNK(src_stat.st_mode) && !follow_symlinks) {
                char target_path[256];
                // appel systeme qui va chercher le path du vrai fichier pointé par le symlink et le passer à target_path
                ssize_t len = readlink(srcPath, target_path, sizeof(target_path) - 1);

                target_path[len] = '\0';
                if (symlink(target_path, dstPath) != 0) {  // crée un lien symbolique qui point vers target
                    perror("Error creating symbolic link");
                    return;
                }
            } 
            
            else {
                src_fd = open(srcPath, O_RDONLY);
                if (src_fd == -1) {
                    perror("Error opening source file");
                    return;
                }
                dst_fd = open(dstPath, O_WRONLY | O_CREAT | O_TRUNC, src_stat.st_mode); // on supprime le fichier, on le recree, et on ecrit dedans ensuite
                if (dst_fd == -1) {
                    perror("Error opening/creating destination file");
                    close(src_fd);
                    return;
                }
             // stocke les donnés de src dans buffer 
             // retourne le nombre de byte lu
                while ((n = read(src_fd, buffer, BUFFER_SIZE)) > 0) { // tant qu il y a encore à lire
                    if (write(dst_fd, buffer, n) != n) {
                        perror("Error writing to destination file");
                        break;
                    }
                }
                // finit quand read retourne 0 


                close(src_fd);
                close(dst_fd);
            }

            if (modify_permissions) {
                chmod(dstPath, src_stat.st_mode);
            }

        } 
        
        else if (modify_permissions && dst_exists == 0) { // dans le cas ou le fichier destination deja existant n a pas à etre touché a part au niveau des permission
            chmod(dstPath, src_stat.st_mode);
        }
    }
}

int main(int argc, char *argv[]) {
    int modify_permissions = 0, follow_symlinks = 1;
    int opt;

    // parse entrée standard
    while ((opt = getopt(argc, argv, "af")) != -1) {
        switch (opt) {
            case 'a': modify_permissions = 1; break; 
            case 'f': follow_symlinks = 0; break;    
            default: 
                fprintf(stderr, "Usage: %s <options> <source> [<source>...] <destination>\n", argv[0]);
                return 1;
        }
    }

    // gérer le premier cas(listing)
    
    if (argc == 2) {
    
        listing(argv[1]);
    
    } else if (argc >= 3) {
    
        
        char *destination = argv[argc - 1];
        struct stat dst_stat;
    
        // vérifie si le dernier argument est un répertoire 
        if (stat(destination, &dst_stat) == -1 || !S_ISDIR(dst_stat.st_mode)) {
            fprintf(stderr, "The destination is not a directory\n");
            return 1;
        }

        // traite chaque fichier/repertoire source iterativement
        for (int i = optind; i < argc - 1; i++) {
            char srcPath[256], dstPath[256];
            
            snprintf(srcPath, sizeof(srcPath), "%s", argv[i]);
            snprintf(dstPath, sizeof(dstPath), "%s/%s", destination, argv[i]);
        

            processPath(srcPath, dstPath, modify_permissions, follow_symlinks);
        }
    }

    return 0;
}
