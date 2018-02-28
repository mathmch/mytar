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

int count_occur(char *path, char c);
void get_path(char buffer[], FILE *tarfile);
void traverse_path(char *path, int is_dir);
mode_t get_mode(FILE *tarfile);
off_t get_size(FILE *tarfile);

/* 1 for verbose/strick on, 0 otherwise */
void extract_file(FILE *tarfile, char *path, int isverbose, int isstrict) {

    char buffer[BLOCK_LENGTH];
    mode_t mode;
    struct utimbuf tm;
    int i;
    int fd;
    int size;
    int blocks;

    if(validate_header(tarfile, isstrict) != 0){
	fprintf(stderr, "Invalid Header\n");
	exit(EXIT_FAILURE);
    }
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
        fseek(tarfile, BLOCK_LENGTH - MTIME_LENGTH - MTIME_OFFSET, SEEK_CUR);
    } else if (S_ISLNK(mode)) {
        fseek(tarfile, LINKNAME_OFFSET -
	      MTIME_LENGTH - MTIME_OFFSET, SEEK_CUR);
        fread(buffer, 1, LINKNAME_LENGTH, tarfile);
        buffer[LINKNAME_LENGTH] = '\0';
        symlink(buffer, path);
        fseek(tarfile, BLOCK_LENGTH -
	      LINKNAME_OFFSET - LINKNAME_LENGTH, SEEK_CUR);
    } else {
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
    if(isverbose)
	printf("%s\n", path);
    if(S_ISDIR(mode)){
        get_path(buffer, tarfile);
        while (strncmp(buffer, path, strlen(path)) == 0) {
            extract_file(tarfile, buffer, isverbose, isstrict);
            get_path(buffer, tarfile);
        }
    }
}

/* searches for archives to extract. If elements is 0, extract all 
 * read path of each header, compare against given paths
 * if archived file is directory, extract everything inside of it */
/* 1 for verbose/strick on, 0 otherwise */
void find_archives(FILE *tarfile, char *paths[],
		   int elements, int isverbose, int isstrict){
    #define MAX_PATH_LENGTH 256
    #define MAX_FIELD_LENGTH 155
    int i;
    
    char actual_path[MAX_PATH_LENGTH];
    
    get_path(actual_path, tarfile);
    if(elements == 0)
	extract_file(tarfile, actual_path, isverbose, isstrict);
    while(actual_path[0] != '\0') {
        int extracted = 0;
        for (i = 0; i < elements; i++) {
            if (paths[i] == NULL)
                continue;
            if (strcmp(actual_path, paths[i]) == 0) {
                extract_file(tarfile, paths[i], isverbose, isstrict);
                extracted++;
                paths[i] = NULL; /* don't search for this path again */
            } else {
                /* check if they named a directory without 
		   putting a '/' at the end */
                int path_length = (int)strlen(actual_path);
                if (actual_path[path_length - 1] == '/'
                    && strlen(paths[i]) == path_length - 1
                    && strncmp(actual_path, paths[i],
			       path_length - 1) == 0) {
                    extract_file(tarfile, actual_path, isverbose, isstrict);
                    extracted++;
                    paths[i] = NULL; /* don't search for this path again */
                }
            }
        }

        if (!extracted) {
            /* extraction moves us along in the tarfile,
             but if we don't extract, we have to go forward anyway */
            mode_t mode = get_mode(tarfile);
            off_t size = S_ISDIR(mode) ? 0 : get_size(tarfile);
            int blocks = size_to_blocks(size);
            /* +1 for the header block */
            fseek(tarfile, BLOCK_LENGTH * (blocks + 1), SEEK_CUR);
        }

        get_path(actual_path, tarfile);
    }
    for(i = 0; i < elements; i++){
	if(paths[i] != NULL)
	    printf("Could not extract: %s\n", paths[i]);
    }
    
}

void traverse_path(char *path, int is_dir){
    char copy[PATH_MAX];
    char *token;
    copy[0] = '\0';
    strcpy(copy, path);
    int count = count_occur(copy, '/');
    if (is_dir)
        count--;
    int i;
    token = strtok(copy, "/");
    for(i = 0; i < count; i++){
        mkdir(token, 0777);
        chdir(token);
        token = strtok(NULL, "/");
    }
    for(i = 0; i < count; i++)
        chdir("..");
}
