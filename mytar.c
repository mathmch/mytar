#include "dirnode.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "acreate.h"
#include "aextract.h"
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include "header.h"
#include "util.h"
#include "listing.h"

void validate_command(int argc, char *argv[]);
int execute_command(int argc, char *argv[]);          

int main(int argc, char *argv[]) {
    validate_command(argc, argv);
    execute_command(argc, argv);
    return 0;
}

void validate_command(int argc, char *argv[]) {
    int length;
    int i;

    if (argc < 3) {
        fprintf(stderr, "Usage: mytar [ctxvS]f tarfile [ path [ ... ] ]\n");
        exit(EXIT_FAILURE);
    }


    if (!strstr(argv[1], "f")) {
        fprintf(stderr, "Usage: mytar [ctxvS]f tarfile [ path [ ... ] ]\n");
        exit(EXIT_FAILURE);
    }

    length = (int)strlen(argv[1]);
    for (i = 0; i < length; i++) {
        if (argv[1][i] != 'c' && argv[1][i] != 't' &&
            argv[1][i] != 'x' && argv[1][i] != 'f' &&
            argv[1][i] != 'v' && argv[1][i] != 'S') {
            fprintf(stderr, "Usage: mytar [ctxvS]f tarfile [ path [ ... ] ]\n");
            exit(EXIT_FAILURE);
        }
    }
}

int execute_command(int argc, char *argv[]) {
    FILE *tarfile;
    char **paths;
    int verbose, strict;
    int i;

    /* create list of paths equal to number of things to extract */
    paths = safe_malloc(sizeof(char*) * (argc-2), "mytar");
    verbose = strstr(argv[1], "v") != 0;
    strict = strstr(argv[1], "S") != 0;

    /* populates the path array */
    for (i = 0; i < argc-3; i++) {
        paths[i] = argv[i+3];
    }
    
    if (strstr(argv[1], "c")) {
        if ((tarfile = fopen(argv[2], "w+")) == NULL) {
            perror(argv[2]);
            exit(EXIT_FAILURE);
        }
        /* create archive */
        archive(tarfile, paths, i, verbose);
        fclose(tarfile);
    } else if (strstr(argv[1], "x")) {
        if ((tarfile = fopen(argv[2], "r")) == NULL) {
            perror(argv[2]);
            exit(EXIT_FAILURE);
        }
        /* extract archive */
        find_archives(tarfile, paths, i, verbose, strict);
        fclose(tarfile);
    } else if (strstr(argv[1], "t")) {
        if ((tarfile = fopen(argv[2], "r")) == NULL) {
            perror(argv[2]);
            exit(EXIT_FAILURE);
        }
        /* list archive */
        find_listings(tarfile, paths, i, verbose, strict);
        fclose(tarfile);
    }

    free(paths);
    return 0;
}
