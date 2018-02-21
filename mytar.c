#include "dirnode.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "acreate.h"

void validate_command(int argc, char *argv[]);
int execute_command(int argc, char *argv[]);
void list_contents(FILE* tarfile, int mode);

int main(int argc, char *argv[]) {
    validate_command(argc, argv);
    execute_command(argc, argv);
    return 0;
}

void validate_command(int argc, char *argv[]){
    int length;
    int i;
    if(argc < 3){
        printf("mytar [ctxvS]f tarfile [ path [ ... ] ]\n");
        exit(EXIT_FAILURE);
    }
    length = strlen(argv[1]);
    for(i = 0; i < length; i++){
        if(argv[1][i] != 'c' && argv[1][i] != 't' &&
           argv[1][i] != 'x' && argv[1][i] != 'v' &&
           argv[1][i] != 'f' && argv[1][i] != 'S'){
            printf("mytar [ctxvS]f tarfile [ path [ ... ] ]\n");
            exit(EXIT_FAILURE);
        }
    }
}

int execute_command(int argc, char *argv[]){
    if(strstr(argv[1], "c")){
        /* create archive */
        if(strstr(argv[1], "v") && strstr(argv[1], "S")){
            /* use verbose and strict */
        }
        if(strstr(argv[1], "v")){
            /* use verbose */
        }
        if(strstr(argv[1], "S")){
            /* use strict */
        }
        /* no flags */
        dirnode *tree = build_tree(argv[3], NULL);
        print_tree(tree);
        write_header(argv[2], tree);
    }
    
    if(strstr(argv[1], "x")){
        /* extract archive */
        if(strstr(argv[1], "v") && strstr(argv[1], "S")){
            /* use verbose and strict */
        }
        if(strstr(argv[1], "v")){
            /* use verbose */
        }
        if(strstr(argv[1], "S")){
            /* use strict */
        }
        /* no flags */
    }

    if(strstr(argv[1], "l")){
        /* list archive */
        if(strstr(argv[1], "v") && strstr(argv[1], "S")){
            /* use verbose and strict */
        }
        if(strstr(argv[1], "v")){
            /* use verbose */
        }
        if(strstr(argv[1], "S")){
            /* use strict */
        }
        /* no flags */
    }
    return 0;
}

/* mode = 0 for standard, verbose otherwise 
   TODO: Put it in a loop so it runs through all headers */
void list_contents(FILE* tarfile, int mode){
    #define PATH_MAX 256
    #define NAME_LENGTH 100
    #define SIZE_OFFSET 124
    #define SIZE_LENTH 12
    #define PREFIX_LENGTH 155
    #define PREFIX_OFFSET 345
    #define BLOCK_SIZE 512
    #define EXTRA_SPACE 12
    int size;
    char path[PATH_MAX];
    char buffer[NAME_LENGTH];

    /* get size */
    fseek(tarfile, SIZE_OFFSET, SEEK_CUR);
    fread(buffer, 1, SIZE_LENGTH, tarfile);
    size = strtol(buffer, NULL, 8);
    /* get name */
    fread(buffer, 1, -SIZE_OFFSET - SIZE_LENGTH, SEEK_CUR);
    /* get prefix */
    fseek(tarfile, PREFIX_OFFSET - NAME_LENGTH, SEEK_CUR);
    fread(path, 1, PREFIX_LENGTH, tarfile);
    /* make full path */
    strcat(path, buffer);
    if(mode){
	/* verbose print */
	
    }else{
	/* standard print */
	puts(path);
    }
    /* go to end of block */
    fseek(tarfile, EXTRA_SPACE, SEEK_CUR);
    /* go to next header */
    fseek(tarfile, size + size%512, SEEK_CUR);
}
