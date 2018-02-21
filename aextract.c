#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include "dirnode.h"
#include "util.h"
#include "aextract.h"

/* TODO: remove unneeded imports */

FILE *extract_files(FILE *tarfile) {
    /* TODO: put this somewhere shared */
    #define MEANINGFUL_MODE_BITS 5
    #define MAX_FIELD_LENGTH 155

    char *file_name;
    char buffer[MAX_FIELD_LENGTH];
    int mode;


    /* TODO: file name */
    file_name = "THIS IS NOT A REAL FILE NAME";
    fseek(tarfile, 100, SEEK_CUR);

    /* mode */
    fread(buffer, 1, MEANINGFUL_MODE_BITS, tarfile);

    return NULL;
}

