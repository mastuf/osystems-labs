#include <getopt.h>
#include <openssl/evp.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>


char* option(int argc, char* argv[], int* isFile) {
    *isFile = 0;
    char* digest = NULL;

    int tflag = 0;
    int fflag = 0;

    int opt;
    while ((opt = getopt(argc, argv, "ft:")) != -1) {
        switch (opt) {
            case 'f':
                fflag = 1;
                break;
            case 't':
                tflag = 1;
                digest = optarg;
                break;
            default:
                fprintf(stderr, "Invalid usage. Correct format: %s [-f] [-t digest_type] <input>\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (tflag == 0) {
        digest = "SHA1";
    }

    *isFile = fflag;
    return digest;
}