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

FILE *extract_file(FILE *tarfile) {
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
    int i;
    char path[MATH_PATH_LENGTH];
    char buffer[MAX_FIELD_LENGTH]; 
    /* this should probably be in a while loop to search the whole archive */
    /* while(not at end of archive) */
    fread(buffer, 1, NAME_LENGTH, tarfile);
    fseek(tarfile, PREFIX_OFFSET - NAME_LENGTH, SEEK_CUR);
    fread(path, 1, PREFIX_LENGTH, tarfile);
    strcat(path, buffer);
    for(i = 0; i < length; i++){
	if(strcmp(path, paths[i]) == 0){
	    extract_file(tarfile);
	}
    }
    
}
