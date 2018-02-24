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
void list_contents(FILE* tarfile, int verbose);
void get_permissions(char permissions[], mode_t mode);
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
        printf("Usage: mytar [ctxvS]f tarfile [ path [ ... ] ]\n");
        exit(EXIT_FAILURE);
    }
    length = strlen(argv[1]);
    for(i = 0; i < length; i++){
        if(argv[1][i] != 'c' && argv[1][i] != 't' &&
           argv[1][i] != 'x' && argv[1][i] != 'v' &&
           argv[1][i] != 'f' && argv[1][i] != 'S'){
            printf("Usage: mytar [ctxvS]f tarfile [ path [ ... ] ]\n");
            exit(EXIT_FAILURE);
        }
    }
}

int execute_command(int argc, char *argv[]){
    #define SIMPLE 0
    #define VERBOSE 1
    FILE *tarfile;
    if(strstr(argv[1], "c")){
	if((tarfile = fopen(argv[2], "w")) == NULL)
	    perror("Opening Tarfile");
	dirnode *tree = build_tree(argv[3], NULL);
        /* create archive */
        if(strstr(argv[1], "v") && strstr(argv[1], "S")){
            /* use verbose and strict */
	    archive(argv[2], tree);
	     print_tree(tree);
        }
        else if(strstr(argv[1], "v")){
            /* use verbose */
	    archive(argv[2], tree);
	     print_tree(tree);
	    
        }
        else if(strstr(argv[1], "S")){
            /* use strict */
	    archive(argv[2], tree);
	}
	else{
	    /* no flags */
	    archive(argv[2], tree);
	}
    }
    
    if(strstr(argv[1], "x")){
	if((tarfile = fopen(argv[2], "r")) == NULL)
	    perror("Opening Tarfile");
        /* extract archive */
        if(strstr(argv[1], "v") && strstr(argv[1], "S")){
            /* use verbose and strict */
        }
        else if(strstr(argv[1], "v")){
            /* use verbose */
        }
        else if(strstr(argv[1], "S")){
            /* use strict */
        }
	else{
	    /* no flags */
	}
    }

    if(strstr(argv[1], "t")){
	if((tarfile = fopen(argv[2], "r")) == NULL)
	    perror("Opening Tarfile");
        /* list archive */
        if(strstr(argv[1], "v") && strstr(argv[1], "S")){
            /* use verbose and strict */
	    list_contents(tarfile, VERBOSE);
        }
        else if(strstr(argv[1], "v")){
            /* use verbose */
	    list_contents(tarfile, VERBOSE);
        }
        else if(strstr(argv[1], "S")){
            /* use strict */
	    list_contents(tarfile, SIMPLE);
        }
	else{
	    /* no flags */
	    list_contents(tarfile, SIMPLE);
	}
    }
    return 0;
}

/* mode = 0 for standard, verbose otherwise 
   TODO: Put it in a loop so it runs through all headers */
void list_contents(FILE* tarfile, int verbose){
    #define HEADER_LENGTH 512
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
    #define PERMISSION_OFFSET 100
    #define OWNER_WIDTH 17
    #define UID_LENGTH 8
    #define GID_LENGTH 8
    #define TIME_LENGTH 12
    #define TIME_OFFSET 136
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
    mode_t mode;
    buffer[0] = '\0';
    fread(buffer, 1, 1, tarfile);
    fseek(tarfile, -1, SEEK_CUR);
    while(buffer[0] != '\0'){
	/* get permissions */
	fseek(tarfile, PERMISSION_OFFSET, SEEK_CUR);
	fread(buffer, 1, PERMISSION_LENGTH, tarfile);
	mode = strtol(buffer, NULL, 8);
	/* get size */
	fseek(tarfile, SIZE_OFFSET - PERMISSION_OFFSET - PERMISSION_LENGTH, SEEK_CUR);
	fread(buffer, 1, SIZE_LENGTH, tarfile);
	size = strtol(buffer, NULL, 8);
	/* get name */
	fseek(tarfile, - SIZE_OFFSET - SIZE_LENGTH, SEEK_CUR);
	fread(buffer, 1, NAME_LENGTH, tarfile);
	/* get prefix */
	fseek(tarfile, PREFIX_OFFSET - NAME_LENGTH, SEEK_CUR);
	fread(path, 1, PREFIX_LENGTH, tarfile);
	/* make full path */
	strcat(path, buffer);
	if(verbose){
	    /* verbose print */
       	    fseek(tarfile, PERMISSION_OFFSET - PREFIX_OFFSET - PREFIX_LENGTH, SEEK_CUR);
	    fread(permissions, 1, PERMISSION_LENGTH, tarfile);
	    get_permissions(permissions, mode);
	
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

	    fseek(tarfile, PREFIX_OFFSET + PREFIX_LENGTH - TIME_OFFSET - TIME_LENGTH, SEEK_CUR); 
	}else{
	    /* standard print */
	    puts(path);
	}
	/* go to end of block */
	fseek(tarfile, EXTRA_SPACE, SEEK_CUR);
	/* go to next header */
	if(!S_ISDIR(mode) && size != 0){
	fseek(tarfile, (size/HEADER_LENGTH)*HEADER_LENGTH + 512, SEEK_CUR);
	}
	fread(buffer, 1, 1, tarfile);
	fseek(tarfile, -1, SEEK_CUR);
    }
}

void get_permissions(char permissions[], mode_t mode){
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
    #define MAX_NAME_LENGTH 8
    #define USER_NAME_LENGTH 32
    char buffer[USER_NAME_LENGTH];
    struct group *gr;
    struct passwd *pw;
    owner[0] = '\0';
    if((pw = getpwuid(uid)) == NULL){
	sprintf(buffer, "%d", uid);
	strcat(owner, buffer);
    }
    else{
	snprintf(buffer, MAX_NAME_LENGTH, "%s", pw->pw_name);
	strcat(owner, buffer);
    }
    strcat(owner, "/");
    if((gr = getgrgid(gid)) == NULL){
	sprintf(buffer, "%d", gid);
	strcat(owner, buffer);
    }
    else{
	snprintf(buffer, MAX_NAME_LENGTH, "%s", gr->gr_name);
	strcat(owner, buffer);
    }
}


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
    sprintf(timestr, "%d-%d-%d %d:%d", year, month, day, hour, min);   
}
