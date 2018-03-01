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

/* make permissions string from mode */
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

/* get owner from uid and gid */
void get_owner(uid_t uid, char uname[], gid_t gid, char gname[], char owner[]){
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

/* converts time into yyyy-mm-dd hh:mm format */
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


/* lists the filename for simple or
 * ls -l style output for verbose */
void list_contents(FILE* tarfile, char path[], int verbose, int strict) {
    #define PERMISSION_WIDTH 11 
    #define TIME_WIDTH 17

    mode_t mode;
    off_t size;
    uid_t uid;
    gid_t gid;
    time_t time;
    char gname[GNAME_LENGTH];
    char uname[UNAME_LENGTH];
    char timestr[TIME_WIDTH];
    char owner[OWNER_WIDTH];
    char buffer[PATH_MAX];
    char permissions[PERMISSION_WIDTH];
    int blocks;

    memset(buffer, 0, PATH_MAX);

    if (validate_header(tarfile, strict) != 0){
        fprintf(stderr, "Invalid Header\n");
        exit(EXIT_FAILURE);
    }
    
    mode = get_mode(tarfile);
    size = get_size(tarfile);

    if (verbose) {
        /* verbose print */
        get_permissions(permissions, mode, tarfile);

        fseek(tarfile, UID_OFFSET, SEEK_CUR);
        safe_fread(owner, 1, UID_LENGTH, tarfile);
        uid = (uid_t)strtol(owner, NULL, 8);
        safe_fread(owner, 1, GID_LENGTH, tarfile);
        gid = (gid_t)strtol(owner, NULL, 8);
        fseek(tarfile, -GID_OFFSET - GID_LENGTH, SEEK_CUR);
        fseek(tarfile, UNAME_OFFSET, SEEK_CUR);
        safe_fread(uname, 1, UNAME_LENGTH, tarfile);
        safe_fread(gname, 1, GNAME_LENGTH, tarfile);
        fseek(tarfile, -GNAME_LENGTH - GNAME_OFFSET, SEEK_CUR);
        get_owner(uid, uname, gid, gname, owner);

        fseek(tarfile, MTIME_OFFSET, SEEK_CUR);
        safe_fread(buffer, 1, MTIME_LENGTH, tarfile);
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
    if (is_reg(tarfile)) {
        blocks = size_to_blocks(size);
        /* +1 for header */
        fseek(tarfile, blocks * (BLOCK_LENGTH + 1), SEEK_CUR);
    } else {
        /* continue extracting until the prefix of the next file
         does not contain this directory's path */
        fseek(tarfile, BLOCK_LENGTH, SEEK_CUR);
        get_path(buffer, tarfile);
        while (strncmp(buffer, path, strlen(path)) == 0) {
            list_contents(tarfile, buffer, verbose, strict);
            get_path(buffer, tarfile);
            if (buffer[0] == '\0')
                return;
        }
    }
}

/* searches for archives to list. if elements is 0, list all.
 * if path searched for is a directory, list its contents. */
void find_listings(FILE *tarfile, char *paths[],
		   int elements, int isverbose, int isstrict){
    char actual_path[PATH_MAX];
    int i;
    
    get_path(actual_path, tarfile);
    if (elements == 0) {
        /* so file specified, so list the whole archive */
        while (actual_path[0] != '\0') {
            list_contents(tarfile, actual_path, isverbose, isstrict);
            get_path(actual_path, tarfile);
        }
    }

    while (actual_path[0] != '\0') {
        int listed = 0;
        for (i = 0; i < elements; i++) {
            if (paths[i] == NULL)
                continue;
            if (strcmp(actual_path, paths[i]) == 0) {
                /* found a path to list */
                list_contents(tarfile, actual_path, isverbose, isstrict);
                listed++;
                paths[i] = NULL; /* don't search for this path again */
            } else {
                /* check if they named a directory without
                 putting a '/' at the end */
                int path_length = (int)strlen(actual_path);
                int ends_with_slash = actual_path[path_length - 1] == '/';
                int lengths_match = strlen(paths[i]) == path_length - 1;
                int paths_match = strncmp(actual_path, paths[i],
                                          path_length - 1) == 0;
                if (ends_with_slash && lengths_match && paths_match) {
                    list_contents(tarfile, actual_path, isverbose, isstrict);
                    listed++;
                    paths[i] = NULL; /* don't search for this path again */
                }
            }
        }

        if (!listed) {
            /* ignore size if not a regular file */
            off_t size = is_reg(tarfile) ? get_size(tarfile) : 0;
            int blocks = size_to_blocks(size);
            /* +1 for the header block */
            fseek(tarfile, BLOCK_LENGTH * (blocks + 1), SEEK_CUR);
        }

        get_path(actual_path, tarfile);
    }
    /* check if any paths could not be listed */
    for (i = 0; i < elements; i++) {
        /* print the paths that could not be found */
        if (paths[i] != NULL)
            printf("Could not list: %s\n", paths[i]);
    }
}
