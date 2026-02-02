#include <getopt.h>
#include <openssl/evp.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "options.h"
#include "digest.h"


int main(int argc, char* argv[]) {
    int isFile;

    char* digest = option(argc, argv, &isFile);
    // "optind" helps skip the options and start directly with the non-option arguments
    for (int i = optind; i < argc; i++) {
        digestZ(argv[i], digest, isFile);
    }

    return 0;
}

