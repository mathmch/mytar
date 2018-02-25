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

/* searches for archives to extract. If elements is 0, extract all 
 * read path of each header, compare against given paths
 * if archived file is directory, extract everything inside of it */
void find_archives(FILE *tarfile, char *paths[], int elements){
    #define MAX_PATH_LENGTH 256
    #define MAX_FIELD_LENGTH 155
    #define NAME_LENGTH 100
    #define PREFIX_OFFSET 345
    #define PREFIX_LENGTH 155
    char path[MATH_PATH_LENGTH];
    char buffer[MAX_FIELD_LENGTH];
    fread(buffer, 1, NAME_LENGTH, tarfile);
    fseek(tarfile, PREFIX_OFFSET - NAME_LENGTH, SEEK_CUR);
    /* build rest of path */
    
    
    
}
