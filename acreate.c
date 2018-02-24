#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include "dirnode.h"
#include "util.h"
#include "acreate.h"

#define PATH_MAX 256
#define BLOCK_SIZE 512

#define NAME_FIELD_LENGTH 100
#define MODE_FIELD_LENGTH 8
#define UID_FIELD_LENGTH 8
#define GID_FIELD_LENGTH 8
#define SIZE_FIELD_LENGTH 12
#define MTIME_FIELD_LENGTH 12
#define CHKSUM_FIELD_LENGTH 8
#define CHKSUM_START_OFFSET 148
#define TYPEFLAG_FIELD_LENGTH 1
#define LINKNAME_FIELD_LENGTH 100
#define MAGIC "ustar"
#define MAGIC_FIELD_LENGTH 6
#define VERSION "00"
#define VERSION_FIELD_LENGTH 2
#define UNAME_FIELD_LENGTH 32
#define GNAME_FIELD_LENGTH 32
#define DEVMAJOR_FIELD_LENGTH 8
#define DEVMINOR_FIELD_LENGTH 8
#define PREFIX_FIELD_LENGTH 155
#define MAX_FIELD_LENGTH 155
#define HEADER_LENGTH 500

void archive(char *file_name, dirnode *tree) {
    FILE *file;
    char buffer[BLOCK_SIZE];
    if ((file = fopen(file_name, "r+")) == NULL) {
        /* TODO: handle error here */
        return;
    }
    archive_helper(file, tree);
    memset(buffer, 0, BLOCK_SIZE);
    fwrite(buffer, 1, BLOCK_SIZE, file);
    fwrite(buffer, 1, BLOCK_SIZE, file);
    fclose(file);
}

void archive_helper(FILE *file, dirnode *tree) {
    int i;
    write_header(file, tree);
    write_contents(file, tree);
    for (i = 0; i < tree->child_count; i++)
        archive_helper(file, tree->children[i]);
}

void write_header(FILE *archive, dirnode *tree) {
    int length;
    char *name;
    char buffer[HEADER_LENGTH], prefix[PREFIX_FIELD_LENGTH];
    int checksum;
    int i;

    memset(buffer, 0, HEADER_LENGTH);

    /* name */
    if (split_path(tree->path_name, prefix, buffer) == -1)
        return; /* TODO: handle the error here some how? */
    length = (int)strlen(buffer); /* length of name */
    fwrite(buffer, 1, length, archive);
    memset(buffer, 0, NAME_FIELD_LENGTH - length);
    fwrite(buffer, 1, NAME_FIELD_LENGTH - length, archive);

    /* mode */
    sprintf(buffer, "%o", tree->sb.st_mode);
    fwrite(buffer, 1, MODE_FIELD_LENGTH, archive);

    /* uid */
    sprintf(buffer, "%o", tree->sb.st_uid);
    fwrite(buffer, 1, UID_FIELD_LENGTH, archive);

    /* gid */
    sprintf(buffer, "%o", tree->sb.st_gid);
    fwrite(buffer, 1, GID_FIELD_LENGTH, archive);

    /* size */
    sprintf(buffer, "%llo", tree->sb.st_size);
    fwrite(buffer, 1, SIZE_FIELD_LENGTH, archive);

    /* mtime */
    sprintf(buffer, "%lo", tree->sb.st_mtimespec.tv_sec);
    fwrite(buffer, 1, MTIME_FIELD_LENGTH, archive);

    /* chksum */
    memset(buffer, ' ', CHKSUM_FIELD_LENGTH);
    fwrite(buffer, 1, CHKSUM_FIELD_LENGTH, archive);

    /* typeflag */
    if (S_ISDIR(tree->sb.st_mode)) {
        buffer[0] = '5';
    } else if (S_ISLNK(tree->sb.st_mode)) {
        buffer[0] = '2';
    } else {
        /* regular file */
        buffer[0] = '0';
    }
    fwrite(buffer, 1, TYPEFLAG_FIELD_LENGTH, archive);

    /* linkname */
    if (S_ISLNK(tree->sb.st_mode)) {
        length = (int)readlink(tree->path_name, buffer, LINKNAME_FIELD_LENGTH);
        if (length < 0) {
            getcwd(buffer, 500);
            puts(buffer);
            perror("symlink");
            exit(EXIT_FAILURE);
        }
        fwrite(buffer, 1, length, archive);
        if (length < LINKNAME_FIELD_LENGTH) {
            memset(buffer, 0, LINKNAME_FIELD_LENGTH - length);
            fwrite(buffer, 1, LINKNAME_FIELD_LENGTH - length, archive);
        }
    } else {
        memset(buffer, 0, LINKNAME_FIELD_LENGTH);
        fwrite(buffer, 1, LINKNAME_FIELD_LENGTH, archive);
    }

    /* magic */
    fwrite(MAGIC, 1, MAGIC_FIELD_LENGTH, archive);

    /* version */
    fwrite(VERSION, 1, VERSION_FIELD_LENGTH, archive);

    /* uname */
    name = getpwuid(tree->sb.st_uid)->pw_name;
    length = (int)strlen(name);
    if (length > UNAME_FIELD_LENGTH - 1) {
        fwrite(name, 1, UNAME_FIELD_LENGTH - 1, archive);
        buffer[0] = '\0';
        fwrite(buffer, 1, 1, archive);
    } else {
        fwrite(name, 1, length, archive);
        memset(buffer, 0, UNAME_FIELD_LENGTH - length);
        fwrite(buffer, 1, UNAME_FIELD_LENGTH - length, archive);
    }

    /* gname */
    name = getgrgid(tree->sb.st_gid)->gr_name;
    length = (int)strlen(name);
    if (length > UNAME_FIELD_LENGTH - 1) {
        fwrite(name, 1, UNAME_FIELD_LENGTH - 1, archive);
        buffer[0] = '\0';
        fwrite(buffer, 1, 1, archive);
    } else {
        fwrite(name, 1, length, archive);
        memset(buffer, 0, UNAME_FIELD_LENGTH - length);
        fwrite(buffer, 1, UNAME_FIELD_LENGTH - length, archive);
    }

    /* devmajor */
    sprintf(buffer, "%o", major(tree->sb.st_dev));
    fwrite(buffer, 1, DEVMAJOR_FIELD_LENGTH, archive);

    /* devminor */
    sprintf(buffer, "%o", minor(tree->sb.st_dev));
    fwrite(buffer, 1, DEVMAJOR_FIELD_LENGTH, archive);

    /* TODO: prefix */
    length = (int)strlen(prefix);
    fwrite(prefix, 1, length, archive);
    memset(buffer, 0, PREFIX_FIELD_LENGTH - length);
    fwrite(buffer, 1, PREFIX_FIELD_LENGTH - length, archive);

    /* checksum calculation and write */
    fseek(archive, -HEADER_LENGTH, SEEK_CUR);
    fread(buffer, 1, HEADER_LENGTH, archive);
    for (checksum = 0, i = 0; i < HEADER_LENGTH; i++) {
        checksum += (unsigned char)buffer[i];
    }
    fseek(archive, -(HEADER_LENGTH - CHKSUM_START_OFFSET), SEEK_CUR);
    sprintf(buffer, "%o", checksum);
    fwrite(buffer, 1, CHKSUM_FIELD_LENGTH, archive);

    /* null padding of header block */
    fseek(archive, HEADER_LENGTH - CHKSUM_START_OFFSET - CHKSUM_FIELD_LENGTH, SEEK_CUR);
    memset(buffer, 0, BLOCK_SIZE - HEADER_LENGTH);
    fwrite(buffer, 1, BLOCK_SIZE - HEADER_LENGTH, archive);
}

/* split a path into prefix and name based on field length.
 * return 0 on success or -1 if the path is too long */
int split_path(char *path, char *prefix, char *name) {
    int path_length = (int)strlen(path);
    int prefix_length;
    char *searchpoint, *slash_split;

    if (path_length > NAME_FIELD_LENGTH + PREFIX_FIELD_LENGTH + 1) {
        /* path too long. (+1 is for the slash that can be removed) */
        return -1;
    }

    if (path_length <= NAME_FIELD_LENGTH) {
        /* whole path fits in the name field */
        strcpy(name, path);
        *prefix = '\0';
        return 0;
    }

    /* figure out where to split */
    searchpoint = path + path_length - NAME_FIELD_LENGTH - 1; /* -1 for the slash */
    slash_split = index(searchpoint, '/');
    if (slash_split == NULL) {
        /* couldn't find a slash to split on */
        return -1;
    }
    prefix_length = (int)(slash_split - path);
    if (prefix_length > PREFIX_FIELD_LENGTH) {
        /* prefix too long */
        return -1;
    }
    strncpy(prefix, path, prefix_length);
    if (prefix_length < PREFIX_FIELD_LENGTH) {
        /* add a null if it fits */
        prefix[prefix_length] = '\0';
    }

    /* +1 to not copy the slash */
    strcpy(name, slash_split + 1);
    return 0;
}

void write_contents(FILE *archive, dirnode *tree) {
    FILE *file;
    char buffer[BLOCK_SIZE];
    size_t length;
    if ((file = fopen(tree->path_name, "r")) == NULL) {
        /* TODO: handle error here */
        return;
    }

    while ((length = fread(buffer, 1, BLOCK_SIZE, file)) > 0) {
        fwrite(buffer, 1, length, archive);
        if (length < BLOCK_SIZE) {
            memset(buffer, 0, BLOCK_SIZE - length);
            fwrite(buffer, 1, BLOCK_SIZE - length, archive);
        }
    }

    fclose(file);
}
