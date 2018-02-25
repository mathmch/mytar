#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <fcntl.h>
#include <unistd.h>
#include <utime.h>
#include <time.h>
#include "dirnode.h"
#include "util.h"
#include "aextract.h"
#include "header.h"

/* TODO: remove unneeded imports */
/* TODO: add strict check for all functions */
 
FILE *extract_file(FILE *tarfile, char *path) {

    char buffer[BLOCK_LENGTH];
    mode_t mode;
    struct utimbuf tm;
    int i;
    int fd;
    int size;
    int blocks;
    
    /* mode */
    fseek(tarfile, -PREFIX_OFFSET-PREFIX_LENGTH + MODE_OFFSET, SEEK_CUR);
    fread(buffer, 1, MODE_LENGTH, tarfile);
    mode = strtol(buffer, NULL, 8);

    /* size */
    fseek(tarfile, SIZE_OFFSET - MODE_LENGTH - MODE_OFFSET, SEEK_CUR);
    fread(buffer, 1, SIZE_LENGTH, tarfile);
    size = strtol(buffer, NULL, 8);

    /* time */
    fread(buffer, 1, MTIME_LENGTH, tarfile);
    tm.actime = time(NULL);
    tm.modtime = strtol(buffer, NULL, 8);
    
    if(S_ISDIR(mode)){
	if(mode & 0111){
	    if(mkdir(path, 0777) < 0)
		perror(path);
	}
	else{
	    if(mkdir(path, 0666) < 0)
		perror(path);
	}
    }
    else{
	if(mode & 0111){
	    if((fd = creat(path, 0777)) < 0)
		perror(path);
	}
        else{
	    if((fd = creat(path, 0666)) < 0)
		perror(path);
	}
	fseek(tarfile, BLOCK_LENGTH - MTIME_OFFSET - MTIME_LENGTH, SEEK_CUR);
	if(size % BLOCK_LENGTH == 0)
	    blocks = (size/BLOCK_LENGTH)*BLOCK_LENGTH;
	else
	    blocks = (size/BLOCK_LENGTH)*BLOCK_LENGTH + BLOCK_LENGTH;
	/* writes the file */
	for(i = 0; i < blocks; i++){
	    fread(buffer, 1, BLOCK_LENGTH, tarfile);
	    if(i == blocks - 1) /* last block */
		write(fd, buffer, size % BLOCK_LENGTH);
	    else
		write(fd, buffer, BLOCK_LENGTH);
	}
	
    }
    utime(path, &tm);
    
    return NULL;
}

/* searches for archives to extract. If elements is 0, extract all 
 * read path of each header, compare against given paths
 * if archived file is directory, extract everything inside of it */
void find_archives(FILE *tarfile, char *paths[], int elements){
    #define MAX_PATH_LENGTH 256
    #define MAX_FIELD_LENGTH 155
    int i;
    
    char path[MAX_PATH_LENGTH];
    char buffer[MAX_FIELD_LENGTH]; 
    /* this should probably be in a while loop to search the whole archive */
    /* while(not at end of archive) */
    fread(buffer, 1, NAME_LENGTH, tarfile);
    fseek(tarfile, PREFIX_OFFSET - NAME_LENGTH, SEEK_CUR);
    fread(path, 1, PREFIX_LENGTH, tarfile);
    strcat(path, buffer);
    for(i = 0; i < elements; i++){
	if(strcmp(path, paths[i]) == 0){
	    extract_file(tarfile, paths[i]);
	}
    }
    
}
