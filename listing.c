#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include "header.h"
#include "util.h"
#include <sys/stat.h>

/* get the permissions string from the mode */
void get_permissions(char permissions[], mode_t mode, FILE *tarfile){
    int index = 0;

    if (is_dir(tarfile))
        permissions[index++] = 'd';
    else if(is_symlink(tarfile))
        permissions[index++] = 'l';
    else
        permissions[index++] = '-';

    permissions[index++] = (mode & S_IRUSR) ? 'r' : '-';
    permissions[index++] = (mode & S_IWUSR) ? 'w' : '-';
    permissions[index++] = (mode & S_IXUSR) ? 'x' : '-';
    permissions[index++] = (mode & S_IRGRP) ? 'r' : '-';
    permissions[index++] = (mode & S_IWGRP) ? 'w' : '-';
    permissions[index++] = (mode & S_IXGRP) ? 'x' : '-';
    permissions[index++] = (mode & S_IROTH) ? 'r' : '-';
    permissions[index++] = (mode & S_IWOTH) ? 'w' : '-';
    permissions[index++] = (mode & S_IXOTH) ? 'x' : '-';
    permissions[index] = '\0';
}

/* get owner string from uid and gid */
void get_owner(uid_t uid, char uname[], gid_t gid, char gname[], char owner[]) {
    #define OWNER_WIDTH 17
    #define MAX_NAME_LENGTH 8
    #define USER_NAME_LENGTH 32

    char buffer[USER_NAME_LENGTH];

    owner[0] = '\0';
    if (uname[0] == '\0') {
        sprintf(buffer, "%d", uid);
        strcat(owner, buffer);
    } else {
        snprintf(buffer, MAX_NAME_LENGTH, "%s", uname);
        strcat(owner, buffer);
    }

    strcat(owner, "/");

    if (gname[0] == '\0') {
        sprintf(buffer, "%d", gid);
        strcat(owner, buffer);
    } else {
        snprintf(buffer, MAX_NAME_LENGTH, "%s", gname);
        strcat(owner, buffer);
    }
}

/* get time string in yyyy:mm:dd hh:mm format */
void get_time(time_t time, char timestr[]){
    struct tm *tm = localtime(&time);
    int year;
    int month;
    int day;
    int hour;
    int min;
    timestr[0] = '\0';
    year = tm->tm_year + 1900;
    month = tm->tm_mon + 1;
    day = tm->tm_mday;
    hour = tm->tm_hour;
    min = tm->tm_min;
    sprintf(timestr, "%d-%02d-%02d %02d:%02d", year, month, day, hour, min);   
}


/* list the contents of a file */
void list_contents(FILE* tarfile, char path[], int isverbose, int isstrict){
    #define EXTRA_SPACE (BLOCK_LENGTH - HEADER_LENGTH)
    #define PERMISSION_WIDTH 11 
    #define OWNER_WIDTH 17
    #define TIME_WIDTH 17
    mode_t mode;
    off_t size;
    time_t time;
    uid_t uid;
    gid_t gid;
    char gname[GNAME_LENGTH];
    char uname[UNAME_LENGTH];
    char timestr[TIME_WIDTH];
    char owner[OWNER_WIDTH];
    char permissions[PERMISSION_WIDTH];
    char buffer[PATH_MAX];
    int blocks;

    buffer[0] = '\0';
    /* get permissions */
    if(validate_header(tarfile, isstrict) != 0){
        fprintf(stderr, "Invalid Header\n");
        exit(EXIT_FAILURE);
    }
    
    mode = get_mode(tarfile);
    /* get size */
    size = get_size(tarfile);
    if(isverbose){
        /* verbose print */
        get_permissions(permissions, mode, tarfile);

        fseek(tarfile, UID_OFFSET, SEEK_CUR);
        fread(owner, 1, UID_LENGTH, tarfile);
        uid = (uid_t)strtol(owner, NULL, 8);
        fread(owner, 1, GID_LENGTH, tarfile);
        gid = (gid_t)strtol(owner, NULL, 8);
        fseek(tarfile, -GID_OFFSET - GID_LENGTH, SEEK_CUR);
        fseek(tarfile, UNAME_OFFSET, SEEK_CUR);
        fread(uname, 1, UNAME_LENGTH, tarfile);
        fread(gname, 1, GNAME_LENGTH, tarfile);
        fseek(tarfile, -GNAME_LENGTH - GNAME_OFFSET, SEEK_CUR);
        get_owner(uid, uname, gid, gname, owner);

        fseek(tarfile, MTIME_OFFSET, SEEK_CUR);
        fread(buffer, 1, MTIME_LENGTH, tarfile);
        time = strtol(buffer, NULL, 8);
        get_time(time, timestr);

        printf("%10s %-17s %8d %16s %s\n", permissions, owner,
               (int)size, timestr, path);

        fseek(tarfile, -MTIME_OFFSET - MTIME_LENGTH, SEEK_CUR);
    } else {
        /* standard print */
        puts(path);
    }

    /* go to next header */
    if(!is_dir(tarfile)){
        blocks = size_to_blocks(size);
        fseek(tarfile, blocks * BLOCK_LENGTH + BLOCK_LENGTH, SEEK_CUR);
    } else {
        /* check next entry */
        fseek(tarfile, BLOCK_LENGTH, SEEK_CUR);
        get_path(buffer, tarfile);
        while (strncmp(buffer, path, strlen(path)) == 0) {
            list_contents(tarfile, buffer, isverbose, isstrict);
            get_path(buffer, tarfile);
            if(buffer[0] == '\0')
                return;
        }
    }
}

void find_listings(FILE *tarfile, char *paths[],
		   int elements, int isverbose, int isstrict){
    #define MAX_PATH_LENGTH 256
    #define MAX_FIELD_LENGTH 155
    int i;
    int listed;
    int path_length;
    char actual_path[MAX_PATH_LENGTH];
    
    get_path(actual_path, tarfile);
    if(elements == 0){
	while(actual_path[0] != '\0') {
	    list_contents(tarfile, actual_path, isverbose, isstrict);
	    get_path(actual_path, tarfile);
	}
    }
    while(actual_path[0] != '\0') {
	listed = 0;
        for (i = 0; i < elements; i++) {
            if (paths[i] == NULL)
                continue;
            if (strcmp(actual_path, paths[i]) == 0) {
                list_contents(tarfile, actual_path, isverbose, isstrict);
                listed++;
                paths[i] = NULL; /* don't search for this path again */
            } else {
                /* check if they named a directory without 
		   putting a '/' at the end */
                path_length = (int)strlen(actual_path);
                if (actual_path[path_length - 1] == '/'
                    && strlen(paths[i]) == path_length - 1
                    && strncmp(actual_path, paths[i],
			       path_length - 1) == 0) {
                    list_contents(tarfile, actual_path, isverbose, isstrict);
                    listed++;
                    paths[i] = NULL; /* don't search for this path again */
                }
            }
        }

        if (!listed) {
            /* extraction moves us along in the tarfile,
             but if we don't extract, we have to go forward anyway */
            off_t size = is_dir(tarfile) ? 0 : get_size(tarfile);
            int blocks = size_to_blocks(size);
            /* +1 for the header block */
            fseek(tarfile, BLOCK_LENGTH * (blocks + 1), SEEK_CUR);
        }

        get_path(actual_path, tarfile);
    }
    for(i = 0; i < elements; i++){
	if(paths[i] != NULL)
	    printf("Could not list: %s\n", paths[i]);
    }
    
}

