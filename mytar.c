#include "dirnode.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void validate_command(int argc, char *argv[]);
int execute_command(int argc, char *argv[]);

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
	dirnode *tree = build_tree(argv[2]);
        char *path_array[30]; /*temp array size for tests */
	print_tree(tree, path_array, 0);
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
}
