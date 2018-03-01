#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include "header.h"
#include "util.h"
#include <sys/stat.h>
#include "listing.h"

void get_permissions(char permissions[], mode_t mode, FILE *tarfile){
    #define TYPE 0
    #define U_R 1
    #define U_W 2
    #define U_X 3
    #define G_R 4
    #define G_W 5
    #define G_X 6
    #define O_R 7
    #define O_W 8
    #define O_X 9
    if (is_dir(tarfile))
    permissions[TYPE] = 'd';
    else if (is_symlink(tarfile))
    permissions[TYPE] = 'l';
    else
        permissions[TYPE] = '-';
    permissions[U_R] = (mode & S_IRUSR) ? 'r' : '-';
    permissions[U_W] = (mode & S_IWUSR) ? 'w' : '-';
    permissions[U_X] = (mode & S_IXUSR) ? 'x' : '-';
    permissions[G_R] = (mode & S_IRGRP) ? 'r' : '-';
    permissions[G_W] = (mode & S_IWGRP) ? 'w' : '-';
    permissions[G_X] = (mode & S_IXGRP) ? 'x' : '-';
    permissions[O_R] = (mode & S_IROTH) ? 'r' : '-';
    permissions[O_W] = (mode & S_IWOTH) ? 'w' : '-';
    permissions[O_X] = (mode & S_IXOTH) ? 'x' : '-';
    permissions[10] = '\0';
}


void get_owner(uid_t uid, char uname[], gid_t gid, char gname[], char owner[]){
    #define OWNER_WIDTH 17
    #define MAX_NAME_LENGTH 8
    #define USER_NAME_LENGTH 32
    char buffer[USER_NAME_LENGTH];
    owner[0] = '\0';
    if (uname[0] == '\0'){
    sprintf(buffer, "%d", uid);
    strcat(owner, buffer);
    }
    else{
    snprintf(buffer, MAX_NAME_LENGTH, "%s", uname);
    strcat(owner, buffer);
    }
    strcat(owner, "/");
    if (gname[0] == '\0'){
    sprintf(buffer, "%d", gid);
    strcat(owner, buffer);
    }
    else{
    snprintf(buffer, MAX_NAME_LENGTH, "%s", gname);
    strcat(owner, buffer);
    }
}

/* converts into yyyy-mm-dd hh:mm format */
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


/* mode = 0 for standard, verbose otherwise 
 * lists the filename for simple or
 * ls -l style output for verbose */
void list_contents(FILE* tarfile, char path[], int isverbose, int isstrict){
    #define PERMISSION_WIDTH 11 
    #define TIME_WIDTH 17
    int size;
    uid_t uid;
    gid_t gid;
    int blocks;
    time_t time;
    char gname[GNAME_LENGTH];
    char uname[UNAME_LENGTH];
    char timestr[TIME_WIDTH];
    char owner[OWNER_WIDTH];
    char buffer[PATH_MAX];
    char permissions[PERMISSION_WIDTH];
    mode_t mode;
    buffer[0] = '\0';
    /* get permissions */
    if (validate_header(tarfile, isstrict) != 0){
    fprintf(stderr, "Invalid Header\n");
    exit(EXIT_FAILURE);
    }
    
    mode = get_mode(tarfile);
    size = get_size(tarfile);
    if (isverbose){
    /* verbose print */
    get_permissions(permissions, mode, tarfile);
    
    fseek(tarfile, UID_OFFSET, SEEK_CUR);
    fread(owner, 1, UID_LENGTH, tarfile);
    uid = strtol(owner, NULL, 8);
    fread(owner, 1, GID_LENGTH, tarfile);
    gid = strtol(owner, NULL, 8);
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
           size, timestr, path);

    fseek(tarfile, -MTIME_OFFSET - MTIME_LENGTH, SEEK_CUR); 
    }else{
    /* standard print */
    puts(path);
    }
    
    /* go to next header */
    if (!is_dir(tarfile)){
    blocks = size_to_blocks(size);
        fseek(tarfile, blocks * BLOCK_LENGTH + BLOCK_LENGTH, SEEK_CUR);
    }
   
    /* next entry */
    else{
    fseek(tarfile, BLOCK_LENGTH, SEEK_CUR);
        get_path(buffer, tarfile);
        while (strncmp(buffer, path, strlen(path)) == 0) {
            list_contents(tarfile, buffer, isverbose, isstrict);
            get_path(buffer, tarfile);
        if (buffer[0] == '\0')
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
    if (elements == 0){
    while (actual_path[0] != '\0') {
        list_contents(tarfile, actual_path, isverbose, isstrict);
        get_path(actual_path, tarfile);
    }
    }
    while (actual_path[0] != '\0') {
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
    /* check if any paths could not be listed */
    for(i = 0; i < elements; i++){
    if(paths[i] != NULL)
        printf("Could not list: %s\n", paths[i]);
    }
    
}

