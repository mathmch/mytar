#include "dirnode.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "acreate.h"
#include <time.h>
#include <pwd.h>
#include <grp.h>


void validate_command(int argc, char *argv[]);
int execute_command(int argc, char *argv[]);
void list_contents(FILE* tarfile, int mode);
void get_permissions(char permissions[]);
void get_owner(uid_t uid, gid_t gid, char owner[]);
void get_time(time_t time, char timestr[]);

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
    #define SIZE_LENGTH 12
    #define PREFIX_LENGTH 155
    #define PREFIX_OFFSET 345
    #define BLOCK_SIZE 512
    #define EXTRA_SPACE 12
    #define PERMISSION_LENGTH 8
    #define PERMISSION_WIDTH 11
    #define PERMISSION_OFFSET 110
    #define OWNER_WIDTH 17
    #define UID_LENGTH 8
    #define GID_LENGTH 8
    #define TIME_LENGTH 12
    #define TIME_WIDTH 16
    int size;
    uid_t uid;
    gid_t gid;
    time_t time;
    char timestr[TIME_WIDTH];
    char owner[OWNER_WIDTH];
    char path[PATH_MAX];
    char buffer[NAME_LENGTH];
    char permissions[PERMISSION_WIDTH];
    

    /* get size */
    fseek(tarfile, SIZE_OFFSET, SEEK_CUR);
    fread(buffer, 1, SIZE_LENGTH, tarfile);
    size = strtol(buffer, NULL, 8);
    /* get name */
    fread(buffer, 1, -SIZE_OFFSET - SIZE_LENGTH, tarfile);
    /* get prefix */
    fseek(tarfile, PREFIX_OFFSET - NAME_LENGTH, SEEK_CUR);
    fread(path, 1, PREFIX_LENGTH, tarfile);
    /* make full path */
    strcat(path, buffer);
    if(mode){
	/* verbose print */
	fseek(tarfile, PERMISSION_OFFSET - PREFIX_OFFSET - PREFIX_LENGTH, SEEK_CUR);
	fread(permissions, 1, PERMISSION_LENGTH, tarfile);
	get_permissions(permissions);
	
	fread(owner, 1, UID_LENGTH, tarfile);
	uid = strtol(owner, NULL, 8);
	fread(owner, 1, GID_LENGTH, tarfile);
	gid = strtol(owner, NULL, 8);
	get_owner(uid, gid, owner);
	
	fseek(tarfile, SIZE_LENGTH, SEEK_CUR);
	fread(buffer, 1, TIME_LENGTH, tarfile);
	time = strtol(buffer, NULL, 8);
	get_time(time, timestr);
	printf("%10s %17s %8d %16s %s\n", permissions, owner, size, timestr, path);
    }else{
	/* standard print */
	puts(path);
    }
    /* go to end of block */
    fseek(tarfile, EXTRA_SPACE, SEEK_CUR);
    /* go to next header */
    fseek(tarfile, size + size%512, SEEK_CUR);
}

void get_permissions(char permissions[]){
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
    mode_t mode;
    mode = strtol(permissions, NULL, 8);
    if(S_ISREG(mode))
	permissions[TYPE] = '-';
    if(S_ISDIR(mode))
	permissions[TYPE] = 'd';
    if(S_ISLNK(mode))
        permissions[TYPE] = 'l';
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
/* figure out how to handled name lengths */
void get_owner(uid_t uid, gid_t gid, char owner[]){
    #define OWNER_WIDTH 17
    #define USER_NAME_LENGTH 32
    int i = 0;
    int length;
    char buffer[USER_NAME_LENGTH];
    struct group *gr;
    struct passwd *pw;
    owner[0] = '\0';
    if((pw = getpwuid(uid)) == NULL){
	sprintf(buffer, "%d", uid);
	strcat(owner, buffer);
    }else{
	/* FINISH */
    }
    strcat(owner, "/");
    if((gr = getgrgid(gid)) == NULL){
	sprintf(buffer, "%d", gid);
	strcat(owner, buffer);
    }else{
	/* FINISH */
    }
    
}

void get_time(time_t time, char timestr[]){
    struct tm *tm = localtime(time);
    int year;
    int month;
    int day;
    int hour;
    int min;
    timestr[0] = '\0';
    year = tm->tm_year + 1970;
    month = tm->tm_mon;
    day = tm->tm_mday;
    hour = tm->tm_hour;
    min = tm->tm_min;
    sprintf(timestr, "%d-%d-%d %d:%d", year, month, day, hour, min);   
}
