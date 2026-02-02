#include <getopt.h>
#include <openssl/evp.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>


// Fonction pour calculer le hachage d'une entrée en utilisant une méthode de hachage donnée.

void digestZ(const char* input, const char* digestType, int isFile) {
    // Vérifier si l'entrée ou le type de hachage est NULL. Quitter le programme s'ils le sont.
    if (!input || !digestType) {
        fprintf(stderr,  "Invalid input\n");
        exit(EXIT_FAILURE);
    }

    // Créer un contexte de hachage pour calculer le hachage.
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) {
        fprintf(stderr, "Failed to create digest context\n");
        exit(EXIT_FAILURE);
    }

    // Obtenir la méthode de hachage à partir du type de hachage fourni (par exemple, SHA1).
    const EVP_MD* hash_m = EVP_get_digestbyname(digestType);
    if (!hash_m ) {
        fprintf(stderr, "Unknown digest type: %s\n", digestType);
        EVP_MD_CTX_free(ctx);
        exit(EXIT_FAILURE);
    }

    // Initialiser le contexte de hachage avec la méthode de hachage sélectionnée.
    if (EVP_DigestInit_ex(ctx, hash_m , NULL) != 1) {
        fprintf(stderr, "Failed to initialize digest\n");
        EVP_MD_CTX_free(ctx);
        exit(EXIT_FAILURE);
    }

    // Tampon pour stocker le hachage calculé et sa longueur.
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_length;

    // Vérifier si l'entrée est un fichier.
    if (isFile) {
        // Ouvrir le fichier. Quitter en cas d'échec.
        FILE* file = fopen(input, "rb");
        if (!file) {
            fprintf(stderr, "Unable to open file: %s\n", input);
            EVP_MD_CTX_free(ctx);
            exit(EXIT_FAILURE);
        }

        // Lire le fichier et mettre à jour le contexte de hachage.
        char buffer[1024];
        for (size_t bytesRead; (bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0; ) {
            EVP_DigestUpdate(ctx, buffer, bytesRead);
        }
        fclose(file);
    } else {
        // Si l'entrée n'est pas un fichier, la traiter comme une chaîne et mettre à jour le contexte de hachage.
        EVP_DigestUpdate(ctx, input, strlen(input));
    }

    // Finaliser le calcul du hachage.
    EVP_DigestFinal_ex(ctx, hash, &hash_length);
    EVP_MD_CTX_free(ctx);

    // Print the computed hash in hexadecimal format.
    for (unsigned int i = 0; i < hash_length; i++) {
        printf("%02x", hash[i]);
    }
    printf("\n");
}
