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

/* archive the files at the paths into the given tar file name */
void archive(FILE *file, char *paths[], int elements, int isverbose) {
    int i;
    dirnode *tree;
    char buffer[BLOCK_LENGTH];

    for (i = 0; i < elements; i++) {
        if ((tree = build_tree(paths[i], NULL)) == NULL)
            continue; /* invalid path, build_tree will print error */
        archive_helper(file, tree);
        if (isverbose)
            print_tree(tree);
        free_tree(tree);
    }

    /* write two empty blocks after archiving */
    memset(buffer, 0, BLOCK_LENGTH);
    fwrite(buffer, 1, BLOCK_LENGTH, file);
    fwrite(buffer, 1, BLOCK_LENGTH, file);
    fclose(file);
}

/* recursive helper function to write header and contents of file */
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

/* write the archive header for the file */
void write_header(FILE *archive, dirnode *tree) {
    #define PERMS(mode) ((mode) & 0777)
    int length;
    char *name;
    char buffer[HEADER_LENGTH], prefix[PREFIX_LENGTH];
    int checksum;

    memset(buffer, 0, HEADER_LENGTH);

    /* name */
    if (split_path(tree->path_name, prefix, buffer) == -1) {
        fprintf(stderr, "%s: name too long", tree->path_name);
        return;
    }
    write_and_pad(buffer, NAME_LENGTH, archive);

    /* mode, including only the permission bits */
    sprintf(buffer, "%07o", PERMS(tree->sb.st_mode));
    fwrite(buffer, 1, MODE_LENGTH, archive);

    /* uid */
    sprintf(buffer, "%07o", (int)tree->sb.st_uid);
    fwrite(buffer, 1, UID_LENGTH, archive);

    /* gid */
    sprintf(buffer, "%07o", tree->sb.st_gid);
    fwrite(buffer, 1, GID_LENGTH, archive);

    /* size */
    sprintf(buffer, "%011o", (unsigned int)tree->sb.st_size);
    fwrite(buffer, 1, SIZE_LENGTH, archive);

    /* mtime */
    sprintf(buffer, "%011o", (unsigned int)tree->sb.st_mtime);
    fwrite(buffer, 1, MTIME_LENGTH, archive);

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
        buffer[length] = '\0';
    } else {
        buffer[0] = '\0';
    }
    write_and_pad(buffer, LINKNAME_LENGTH, archive);

    /* magic */
    fwrite(MAGIC, 1, MAGIC_LENGTH, archive);

    /* version */
    fwrite(VERSION, 1, VERSION_LENGTH, archive);

    /* uname */
    name = getpwuid(tree->sb.st_uid)->pw_name;
    strcpy(buffer, name);
    buffer[UNAME_LENGTH] = '\0'; /* truncate if necessary */
    write_and_pad(buffer, UNAME_LENGTH, archive);

    /* gname */
    name = getgrgid(tree->sb.st_gid)->gr_name;
    strcpy(buffer, name);
    buffer[GNAME_LENGTH] = '\0'; /* truncate if necessary */
    write_and_pad(buffer, GNAME_LENGTH, archive);

    /* devmajor and devminor */
    if (S_ISCHR(tree->sb.st_mode) || S_ISBLK(tree->sb.st_mode)) {
        /* we don't support these file types, so this won't
         actually get called, but... */
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
    checksum = compute_checksum(archive);
    fseek(archive, CHKSUM_OFFSET, SEEK_CUR);
    sprintf(buffer, "%07o", checksum);
    fwrite(buffer, 1, CHKSUM_LENGTH, archive);

    /* null padding at end of header block */
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

    if (path_length > NAME_LENGTH + PREFIX_LENGTH + 1)
        return -1; /* path too long (+1 for extra slash to remove */

    if (path_length <= NAME_LENGTH) {
        /* whole path fits in the name field */
        strcpy(name, path);
        *prefix = '\0';
        return 0;
    }

    /* figure out where to split */
    searchpoint = path + path_length - NAME_LENGTH - 1; /* -1 for the slash */
    slash_split = index(searchpoint, '/');
    if (slash_split == NULL)
        return -1; /* couldn't find a slash to split on */

    prefix_length = (int)(slash_split - path);
    if (prefix_length > PREFIX_LENGTH)
        return -1; /* prefix too long */

    strncpy(prefix, path, prefix_length);
    if (prefix_length < PREFIX_LENGTH)
        prefix[prefix_length] = '\0'; /* add a null if it fits */

    /* +1 to not copy the slash */
    strcpy(name, slash_split + 1);
    return 0;
}

/* write the contents of the file to the archive */
void write_contents(FILE *archive, dirnode *tree) {
    FILE *file;
    char buffer[BLOCK_LENGTH + 1];
    size_t length;
    if ((file = fopen(tree->path_name, "r")) == NULL) {
        perror(tree->path_name);
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
