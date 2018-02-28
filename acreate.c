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
#include "header.h"

void archive(char *file_name, dirnode *tree) {
    FILE *file;
    char buffer[BLOCK_LENGTH];
    if ((file = fopen(file_name, "w+")) == NULL) {
        /* TODO: handle error here */
        return;
    }
    archive_helper(file, tree);
    memset(buffer, 0, BLOCK_LENGTH);
    fwrite(buffer, 1, BLOCK_LENGTH, file);
    fwrite(buffer, 1, BLOCK_LENGTH, file);
    fclose(file);
}

void archive_helper(FILE *file, dirnode *tree) {
    int i;
    if (!S_ISREG(tree->sb.st_mode) &&
	!S_ISDIR(tree->sb.st_mode) && !S_ISLNK(tree->sb.st_mode)) {
        fprintf(stderr, "error: %s: unsupported file type", tree->path_name);
        return;
    }
    write_header(file, tree);
    write_contents(file, tree);
    for (i = 0; i < tree->child_count; i++)
        archive_helper(file, tree->children[i]);
}

void write_header(FILE *archive, dirnode *tree) {
    #define MODE_PERMS(x) ((x) & 0777)
    int length;
    char *name;
    char buffer[HEADER_LENGTH], prefix[PREFIX_LENGTH];
    int checksum;
    int i;

    memset(buffer, 0, HEADER_LENGTH);

    /* name */
    if (split_path(tree->path_name, prefix, buffer) == -1)
        return; /* TODO: handle the error here some how? */
    write_and_pad(buffer, NAME_LENGTH, archive);

    /* mode, including only the permission bits */
    sprintf(buffer, "%07o", MODE_PERMS(tree->sb.st_mode));
    write_and_pad(buffer, MODE_LENGTH, archive);

    /* uid */
    sprintf(buffer, "%07o", (int)tree->sb.st_uid);
    write_and_pad(buffer, UID_LENGTH, archive);

    /* gid */
    sprintf(buffer, "%07o", tree->sb.st_gid);
    write_and_pad(buffer, GID_LENGTH, archive);

    /* size */
    sprintf(buffer, "%011o", (unsigned int)tree->sb.st_size);
    write_and_pad(buffer, SIZE_LENGTH, archive);

    /* mtime */
    sprintf(buffer, "%011o", (unsigned int)tree->sb.st_mtime);
    write_and_pad(buffer, MTIME_LENGTH, archive);

    /* chksum */
    memset(buffer, ' ', CHKSUM_LENGTH);
    fwrite(buffer, 1, CHKSUM_LENGTH, archive);

    /* typeflag */
    if (S_ISDIR(tree->sb.st_mode)) {
        buffer[0] = '5';
    } else if (S_ISLNK(tree->sb.st_mode)) {
        buffer[0] = '2';
    } else {
        /* regular file */
        buffer[0] = '0';
    }
    fwrite(buffer, 1, TYPEFLAG_LENGTH, archive);

    /* linkname */
    if (S_ISLNK(tree->sb.st_mode)) {
        length = (int)readlink(tree->path_name, buffer, LINKNAME_LENGTH);
        fwrite(buffer, 1, length, archive);
        if (length < LINKNAME_LENGTH) {
            memset(buffer, 0, LINKNAME_LENGTH - length);
            fwrite(buffer, 1, LINKNAME_LENGTH - length, archive);
        }
    } else {
        memset(buffer, 0, LINKNAME_LENGTH);
        fwrite(buffer, 1, LINKNAME_LENGTH, archive);
    }

    /* magic */
    fwrite(MAGIC, 1, MAGIC_LENGTH, archive);

    /* version */
    fwrite(VERSION, 1, VERSION_LENGTH, archive);

    /* uname */
    name = getpwuid(tree->sb.st_uid)->pw_name;
    length = (int)strlen(name);
    if (length > UNAME_LENGTH - 1) {
        fwrite(name, 1, UNAME_LENGTH - 1, archive);
        buffer[0] = '\0';
        fwrite(buffer, 1, 1, archive);
    } else {
        fwrite(name, 1, length, archive);
        memset(buffer, 0, UNAME_LENGTH - length);
        fwrite(buffer, 1, UNAME_LENGTH - length, archive);
    }

    /* gname */
    name = getgrgid(tree->sb.st_gid)->gr_name;
    length = (int)strlen(name);
    if (length > UNAME_LENGTH - 1) {
        fwrite(name, 1, UNAME_LENGTH - 1, archive);
        buffer[0] = '\0';
        fwrite(buffer, 1, 1, archive);
    } else {
        fwrite(name, 1, length, archive);
        memset(buffer, 0, UNAME_LENGTH - length);
        fwrite(buffer, 1, UNAME_LENGTH - length, archive);
    }

    /* devmajor and devminor */
    if (S_ISCHR(tree->sb.st_mode) || S_ISBLK(tree->sb.st_mode)) {
        sprintf(buffer, "%07o", major(tree->sb.st_dev));
        write_and_pad(buffer, DEVMAJOR_LENGTH, archive);
        sprintf(buffer, "%07o", minor(tree->sb.st_dev));
        write_and_pad(buffer, DEVMINOR_LENGTH, archive);
    } else {
        buffer[0] = '\0';
        write_and_pad(buffer, DEVMAJOR_LENGTH, archive);
        write_and_pad(buffer, DEVMINOR_LENGTH, archive);
    }

    /* prefix */
    write_and_pad(prefix, PREFIX_LENGTH, archive);

    /* checksum calculation and write */
    fseek(archive, -HEADER_LENGTH, SEEK_CUR);
    fread(buffer, 1, HEADER_LENGTH, archive);
    for (checksum = 0, i = 0; i < HEADER_LENGTH; i++) {
        checksum += (unsigned char)buffer[i];
    }
    fseek(archive, -(HEADER_LENGTH - CHKSUM_OFFSET), SEEK_CUR);
    sprintf(buffer, "%07o", checksum);
    fwrite(buffer, 1, CHKSUM_LENGTH, archive);

    /* null padding of header block */
    fseek(archive, HEADER_LENGTH - CHKSUM_OFFSET - CHKSUM_LENGTH, SEEK_CUR);
    memset(buffer, 0, BLOCK_LENGTH - HEADER_LENGTH);
    fwrite(buffer, 1, BLOCK_LENGTH - HEADER_LENGTH, archive);
}

/* split a path into prefix and name based on field length.
 * return 0 on success or -1 if the path is too long */
int split_path(char *path, char *prefix, char *name) {
    int path_length = (int)strlen(path);
    int prefix_length;
    char *searchpoint, *slash_split;

    if (path_length > NAME_LENGTH + PREFIX_LENGTH + 1) {
        /* path too long. (+1 is for the slash that can be removed) */
        return -1;
    }

    if (path_length <= NAME_LENGTH) {
        /* whole path fits in the name field */
        strcpy(name, path);
        *prefix = '\0';
        return 0;
    }

    /* figure out where to split */
    searchpoint = path + path_length - NAME_LENGTH - 1; /* -1 for the slash */
    slash_split = index(searchpoint, '/');
    if (slash_split == NULL) {
        /* couldn't find a slash to split on */
        return -1;
    }
    prefix_length = (int)(slash_split - path);
    if (prefix_length > PREFIX_LENGTH) {
        /* prefix too long */
        return -1;
    }
    strncpy(prefix, path, prefix_length);
    if (prefix_length < PREFIX_LENGTH) {
        /* add a null if it fits */
        prefix[prefix_length] = '\0';
    }

    /* +1 to not copy the slash */
    strcpy(name, slash_split + 1);
    return 0;
}

void write_contents(FILE *archive, dirnode *tree) {
    FILE *file;
    char buffer[BLOCK_LENGTH + 1];
    size_t length;
    if ((file = fopen(tree->path_name, "r")) == NULL) {
        /* TODO: handle error here */
        return;
    }

    while ((length = fread(buffer, 1, BLOCK_LENGTH, file)) > 0) {
        buffer[length] = '\0';
        write_and_pad(buffer, BLOCK_LENGTH, archive);
    }

    fclose(file);
}

void write_and_pad(char *buffer, int num_to_write, FILE *file) {
    int length = (int)strlen(buffer);
    fwrite(buffer, 1, length, file);
    if (length < num_to_write) {
        memset(buffer, 0, num_to_write - length);
        fwrite(buffer, 1, num_to_write - length, file);
    }
}
