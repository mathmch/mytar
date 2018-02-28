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

#define STRICT 1
#define NON_STRICT 0
#define VERBOSE 1
#define NON_VERBOSE 0
void validate_command(int argc, char *argv[]);
int execute_command(int argc, char *argv[]);
void list_contents(FILE* tarfile, int isverbose, int isstrict);
void get_permissions(char permissions[], mode_t mode);
void get_owner(uid_t uid, gid_t gid, char owner[]);
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

/* mode = 0 for standard, verbose otherwise */
void list_contents(FILE* tarfile, int isverbose, int isstrict){
    #define PATH_MAX 256
    #define EXTRA_SPACE 12
    #define PERMISSION_WIDTH 11 
    #define OWNER_WIDTH 17
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
    /* get permissions */
    if(validate_header(tarfile, isstrict) != 0){
	fprintf(stderr, "Invalid Header\n");
	exit(EXIT_FAILURE);
    }
    fseek(tarfile, MODE_OFFSET, SEEK_CUR);
    fread(buffer, 1, MODE_LENGTH, tarfile);
    mode = strtol(buffer, NULL, 8);
    /* get size */
    fseek(tarfile, SIZE_OFFSET - MODE_OFFSET - MODE_LENGTH, SEEK_CUR);
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
    if(isverbose){
	/* verbose print */
	fseek(tarfile, MODE_OFFSET - PREFIX_OFFSET -
	      PREFIX_LENGTH, SEEK_CUR);
	fread(permissions, 1, MODE_LENGTH, tarfile);
	get_permissions(permissions, mode);

	fread(owner, 1, UID_LENGTH, tarfile);
	uid = strtol(owner, NULL, 8);
	fread(owner, 1, GID_LENGTH, tarfile);
	gid = strtol(owner, NULL, 8);
	get_owner(uid, gid, owner);

	fseek(tarfile, SIZE_LENGTH, SEEK_CUR);
	fread(buffer, 1, MTIME_LENGTH, tarfile);
	time = strtol(buffer, NULL, 8);
	get_time(time, timestr);

	printf("%10s %17s %8d %16s %s\n", permissions, owner,
	       size, timestr, path);

	fseek(tarfile, PREFIX_OFFSET + PREFIX_LENGTH -
	      MTIME_OFFSET - MTIME_LENGTH, SEEK_CUR); 
    }else{
	/* standard print */
	puts(path);
    }
    /* go to end of block */
    fseek(tarfile, EXTRA_SPACE, SEEK_CUR);
    /* go to next header */
    if(!S_ISDIR(mode)){
	if(size % BLOCK_LENGTH == 0)
	    fseek(tarfile, (size/BLOCK_LENGTH)*BLOCK_LENGTH, SEEK_CUR);
	else
	    fseek(tarfile, (size/BLOCK_LENGTH)*BLOCK_LENGTH
		  + BLOCK_LENGTH, SEEK_CUR);
    }
    if(S_ISDIR(mode)){
        get_path(buffer, tarfile);
        while (strncmp(buffer, path, strlen(path)) == 0) {
            list_contents(tarfile, isverbose, isstrict);
            get_path(buffer, tarfile);
	    if(buffer[0] == '\0')
		return;
        }
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

void find_listings(FILE *tarfile, char *paths[],
		   int elements, int isverbose, int isstrict){
    #define MAX_PATH_LENGTH 256
    #define MAX_FIELD_LENGTH 155
    int i;
    int listed = 0;
    int path_length;
    char actual_path[MAX_PATH_LENGTH];
    
    get_path(actual_path, tarfile);
    if(elements == 0)
        list_contents(tarfile, isverbose, isstrict);
    while(actual_path[0] != '\0' && listed != elements) {
        for (i = 0; i < elements; i++) {
            if (paths[i] == NULL)
                continue;
            if (strcmp(actual_path, paths[i]) == 0) {
                list_contents(tarfile, isverbose, isstrict);
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
                    list_contents(tarfile, isverbose, isstrict);
                    listed++;
                    paths[i] = NULL; /* don't search for this path again */
                }
            }
        }

        if (!listed) {
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
	    printf("Could not list: %s\n", paths[i]);
    }
    
}

