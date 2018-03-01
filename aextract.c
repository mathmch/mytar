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

#define ANY_X(mode) ((mode) & 0111)
#define ALL_RWX 0777
#define ALL_RW 0666

/* extracts the file with the given path */
void extract_file(FILE *tarfile, char *path, int verbose, int strict) {
    char buffer[BLOCK_LENGTH];
    mode_t mode;
    off_t size;
    struct utimbuf tm;
    int fd, blocks, i;

    if (validate_header(tarfile, strict) != 0) {
        fprintf(stderr, "Invalid Header\n");
        exit(EXIT_FAILURE);
    }

    mode = get_mode(tarfile);
    size = get_size(tarfile);

    if (is_dir(tarfile)) {
        traverse_path(path, 1);
        if (ANY_X(mode)) {
            mkdir(path, ALL_RWX); /* ignore error if already exists */
        } else {
            mkdir(path, ALL_RW); /* ignore error if already exists */
        }
    } else if (is_symlink(tarfile)) {
        traverse_path(path, 0);
        get_linkname(buffer, tarfile);
        symlink(buffer, path);
        fseek(tarfile, BLOCK_LENGTH, SEEK_CUR);
    } else {
        traverse_path(path, 0);
        if (ANY_X(mode)) {
            if ((fd = creat(path, ALL_RWX)) != 0)
                perror(path);
        } else {
            if ((fd = creat(path, ALL_RW)) != 0)
                perror(path);
        }

        /* go to file contents and write the file */
        fseek(tarfile, BLOCK_LENGTH, SEEK_CUR);
        blocks = size_to_blocks(size);
        for (i = 0; i < blocks; i++) {
            safe_fread(buffer, 1, BLOCK_LENGTH, tarfile);
            if (i == blocks - 1) /* last block */
                write(fd, buffer, size % BLOCK_LENGTH);
            else
                write(fd, buffer, BLOCK_LENGTH);
        }

        close(fd);
    }

    tm.actime = time(NULL);
    tm.modtime = get_mtime(tarfile);
    utime(path, &tm);

    if (verbose)
        printf("%s\n", path);

    if (is_dir(tarfile)) {
        /* continue extracting until the prefix of the next file
         does not contain this directory's path */
        int path_length;
        fseek(tarfile, BLOCK_LENGTH, SEEK_CUR); /* jump to next header */
        path_length = (int)strlen(path);
        get_path(buffer, tarfile);
        while (strncmp(buffer, path, path_length) == 0) {
            extract_file(tarfile, buffer, verbose, strict);
            get_path(buffer, tarfile);
        }
    }
}

/* searches for archives to extract. If elements is 0, extract all 
 * read path of each header, compare against given paths
 * if archived file is directory, extract everything inside of it */
/* 1 for verbose/strick on, 0 otherwise */
void find_archives(FILE *tarfile, char *paths[],
		   int elements, int verbose, int strict) {
    char actual_path[PATH_MAX];
    int i;
    
    get_path(actual_path, tarfile);
    if (elements == 0) {
        /* no file specified, so extract the whole archive */
        while (actual_path[0] != '\0') {
            extract_file(tarfile, actual_path, verbose, strict);
            get_path(actual_path, tarfile);
        }
    }

    while (actual_path[0] != '\0') {
        int extracted = 0;
        for (i = 0; i < elements; i++) {
            if (paths[i] == NULL)
                continue;
            if (strcmp(actual_path, paths[i]) == 0) {
                /* found the path to extract */
                extract_file(tarfile, paths[i], verbose, strict);
                extracted++;
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
                    extract_file(tarfile, actual_path, verbose, strict);
                    extracted++;
                    paths[i] = NULL; /* don't search for this path again */
                }
            }
        }

        /* extraction moves us along in the tarfile,
         but if we don't extract, we have to go forward anyway */
        if (!extracted) {
            /* ignore size if not a regular file */
            off_t size = is_reg(tarfile) ? get_size(tarfile) : 0;
            int blocks = size_to_blocks(size);
            /* +1 for the header block */
            fseek(tarfile, BLOCK_LENGTH * (blocks + 1), SEEK_CUR);
        }

        get_path(actual_path, tarfile);
    }

    for (i = 0; i < elements; i++) {
        /* print the paths that could not be found */
        if (paths[i] != NULL)
            fprintf(stderr, "Could not extract: %s\n", paths[i]);
    }
}

/* makes all directories leading up to the path */
void traverse_path(char *path, int is_dir) {
    char copy[PATH_MAX], *token;
    int count, i;

    copy[0] = '\0';
    strcpy(copy, path);
    count = count_occur(copy, '/');
    if (is_dir)
        count--; /* account for extra slash in directory name */

    token = strtok(copy, "/");
    for (i = 0; i < count; i++) {
        mkdir(token, ALL_RWX);
        chdir(token);
        token = strtok(NULL, "/");
    }

    for (i = 0; i < count; i++)
        chdir(".."); /* go back up for each slash */
}
