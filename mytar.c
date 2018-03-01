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

#define STRICT 1
#define NON_STRICT 0
#define VERBOSE 1
#define NON_VERBOSE 0
void validate_command(int argc, char *argv[]);
int execute_command(int argc, char *argv[]);
void list_contents(FILE* tarfile, char path[], int isverbose, int isstrict);
void get_permissions(char permissions[], mode_t mode, FILE *tarfile);
void get_owner(uid_t uid, char uname[], gid_t gid, char gname[], char owner[]);
void get_time(time_t time, char timestr[]);
void find_listings(FILE *tarfile, char *paths[],
		   int elements, int isverbose, int isstrict);          

int main(int argc, char *argv[]) {
    validate_command(argc, argv);
    execute_command(argc, argv);
    return 0;
}

void validate_command(int argc, char *argv[]){
    int length;
    int i;
    if(argc < 3){
        fprintf(stderr, "Usage: mytar [ctxvS]f tarfile [ path [ ... ] ]\n");
        exit(EXIT_FAILURE);
    }
    length = strlen(argv[1]);
    if(!strstr(argv[1], "f")){
	fprintf(stderr, "Usage: mytar [ctxvS]f tarfile [ path [ ... ] ]\n");
        exit(EXIT_FAILURE);
    }   
    for(i = 0; i < length; i++){
        if(argv[1][i] != 'c' && argv[1][i] != 't' &&
           argv[1][i] != 'x' && argv[1][i] != 'f' &&
	   argv[1][i] != 'v' && argv[1][i] != 'S'){
            fprintf(stderr, "Usage: mytar [ctxvS]f tarfile [ path [ ... ] ]\n");
            exit(EXIT_FAILURE);
        }
    }
}

int execute_command(int argc, char *argv[]){
    FILE *tarfile;
    int i;
    dirnode *tree;
    int isverbose;
    int isstrict;
    char **paths;
    /* create list of paths equal to number of things to extract */
    paths = (char**)malloc(sizeof(char*)*(argc-2));
    isverbose = (strstr(argv[1], "v") != 0) ? VERBOSE : NON_VERBOSE;
    isstrict = (strstr(argv[1], "S") != 0) ? STRICT: NON_STRICT;
    
    if(strstr(argv[1], "c")){
	/* TODO: figure out what strict should do, if anything */
	if((tarfile = fopen(argv[2], "w")) == NULL)
	    perror("Opening Tarfile");
	tree = build_tree(argv[3], NULL);
        /* create archive */
        if(isverbose){
            /* use verbose and strict */
	    archive(argv[2], tree);
	    print_tree(tree);
	}else{
	    archive(argv[2], tree);
	}
    }
    
    /* populates the path array */
    for(i = 0; i < argc-3; i++){
	paths[i] = argv[i+3];
    }
    
    if(strstr(argv[1], "x")){
	if((tarfile = fopen(argv[2], "r")) == NULL)
	    perror("Opening Tarfile");
	
        /* extract archive */
	find_archives(tarfile, paths, i, isverbose, isstrict);
    }

    if(strstr(argv[1], "t")){
	if((tarfile = fopen(argv[2], "r")) == NULL)
	    perror("Opening Tarfile");
        /* list archive */
        find_listings(tarfile, paths, i, isverbose, isstrict);
    }
    free(paths);
    return 0;
}
