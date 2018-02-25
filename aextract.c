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
 
void extract_file(FILE *tarfile, char *path) {

    char buffer[BLOCK_LENGTH];
    mode_t mode;
    struct utimbuf tm;
    int i;
    int fd;
    int size;
    int blocks;
    
    /* mode */
    fseek(tarfile, MODE_OFFSET, SEEK_CUR);
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
	traverse_path(path, 1);
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
	traverse_path(path, 0);
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
	    blocks = (size/BLOCK_LENGTH)*BLOCK_LENGTH + 1;
	/* writes the file */
	for(i = 0; i < blocks; i++){
	    fread(buffer, 1, BLOCK_LENGTH, tarfile);
	    if(i == blocks - 1) /* last block */
		write(fd, buffer, size % BLOCK_LENGTH);
	    else
		write(fd, buffer, BLOCK_LENGTH);
	}
	close(fd);
    }
    utime(path, &tm);
    if(S_ISDIR(mode)){
	fseek(tarfile, BLOCK_LENGTH - MTIME_LENGTH _ MTIME_OFFSET, SEEK_CUR);
	get_path(buffer, tarfile);
	if(strncmp(buffer, path, strlen(path)) == 0){
	    extract_file(tarfile, buffer);
    }
}

/* searches for archives to extract. If elements is 0, extract all 
 * read path of each header, compare against given paths
 * if archived file is directory, extract everything inside of it */
/* this function is pretty broken */
void find_archives(FILE *tarfile, char *paths[], int elements){
    #define MAX_PATH_LENGTH 256
    #define MAX_FIELD_LENGTH 155
    int i;
    
    char path[MAX_PATH_LENGTH];
    
    getpath(path, tarfile);
    while(path[0] != '\0'){
	for(i = 0; i < elements; i++){
	    int path_length = strlen(buffer);
	    if (buffer[buffer_length - 1] == '/')
		
	    if(strcmp(path, paths[i]) == 0 || strncmp(path, paths[i], strlen(paths[i]) - 1) == 0){
		extract_file(tarfile, paths[i]);
	    }
	}
	getpath(path, tarfile);
    }
    
}

void traverse_path(char *path, int is_dir){
    char *token;
    int count = count_occur(path, '/');
    int i;
    token = strtok(path, "/");
    for(i = 0; i < count; i++){	
	mkdir(token, 0777);
	chdir(token);
	token = strtok(NULL, "/");
    }
    if(is_dir && *token != '\0'){
        mkdir(token, 0777);
    }
    for(i = 0; i < count; i++)
	chdir("..");   
}

int count_occur(char *path, char c){
    int i;
    int count = 0;;
    for(i = 0; path[i] = '\0'; i++){
	if(path[i] == c)
	    count++;
    }
    return count;
}

/* assume at start of header, resets to start of header */
void  get_path(char buffer[], FILE *tarfile){
    char name[NAME_LENGTH + 1];
    fseek(tarfile, PREFIX_OFFSET, SEEK_CUR);
    fread(buffer, 1, PREFIX_LENGTH, tarfile);
    if(buffer[0] != 0)
	strcat(buffer, "/");
    fseek(tarfile, -PREFIX_OFFSET - PREFIX_LENGTH, SEEK_CUR);
    fread(name, 1, NAME_LENGTH, tarfile);
    name[NAME_LENGTH] = '\0';
    strcat(buffer, name);
    fseek(tarfile, -NAME_LENGTH, SEEK_CUR);
}
