#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include "dirnode.h"
#include "util.h"
#include "acreate.h"

#define PATH_LENGTH 256
#define BLOCK_SIZE 512

void write_header(char *file_name, dirnode *tree) {
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

    FILE *file;
    unsigned long length;
    char *name;
    char buffer[HEADER_LENGTH];
    int checksum;
    int i;

    if ((file = fopen(file_name, "w+")) == NULL) {
        perror("TESTING");
        return;
    }

    memset(buffer, 0, HEADER_LENGTH);

    /* name */
    length = strlen(file_name);
    fwrite(file_name, 1, length, file);
    fwrite(buffer, 1, NAME_FIELD_LENGTH - length, file);

    /* mode */
    sprintf(buffer, "%o", tree->sb.st_mode);
    fwrite(buffer, 1, MODE_FIELD_LENGTH, file);

    /* uid */
    sprintf(buffer, "%o", tree->sb.st_uid);
    fwrite(buffer, 1, UID_FIELD_LENGTH, file);

    /* gid */
    sprintf(buffer, "%o", tree->sb.st_gid);
    fwrite(buffer, 1, GID_FIELD_LENGTH, file);

    /* size */
    sprintf(buffer, "%llo", tree->sb.st_size);
    fwrite(buffer, 1, SIZE_FIELD_LENGTH, file);

    /* mtime */
    sprintf(buffer, "%lo", tree->sb.st_mtimespec.tv_sec);
    fwrite(buffer, 1, MTIME_FIELD_LENGTH, file);

    /* chksum */
    for (i = 0; i < 8; i++) {
        buffer[i] = ' ';
    }
    fwrite(buffer, 1, CHKSUM_FIELD_LENGTH, file);

    /* typeflag */
    if (S_ISDIR(tree->sb.st_mode)) {
        buffer[0] = '5';
    } else if (S_ISLNK(tree->sb.st_mode)) {
        buffer[0] = '2';
    } else {
        /* regular file */
        buffer[0] = '0';
    }
    fwrite(buffer, 1, TYPEFLAG_FIELD_LENGTH, file);

    /* TODO: linkname */
    memset(buffer, 15, LINKNAME_FIELD_LENGTH);
    fwrite(buffer, 1, LINKNAME_FIELD_LENGTH, file);

    /* magic */
    fwrite(MAGIC, 1, MAGIC_FIELD_LENGTH, file);

    /* version */
    fwrite(VERSION, 1, VERSION_FIELD_LENGTH, file);

    /* uname */
    name = getpwuid(tree->sb.st_uid)->pw_name;
    length = strlen(name);
    if (length > UNAME_FIELD_LENGTH - 1) {
        fwrite(name, 1, UNAME_FIELD_LENGTH - 1, file);
        buffer[0] = '\0';
        fwrite(buffer, 1, 1, file);
    } else {
        fwrite(name, 1, length, file);
        memset(buffer, 0, UNAME_FIELD_LENGTH - length);
        fwrite(buffer, 1, UNAME_FIELD_LENGTH - length, file);
    }

    /* gname */
    name = getgrgid(tree->sb.st_gid)->gr_name;
    length = strlen(name);
    if (length > UNAME_FIELD_LENGTH - 1) {
        fwrite(name, 1, UNAME_FIELD_LENGTH - 1, file);
        buffer[0] = '\0';
        fwrite(buffer, 1, 1, file);
    } else {
        fwrite(name, 1, length, file);
        memset(buffer, 0, UNAME_FIELD_LENGTH - length);
        fwrite(buffer, 1, UNAME_FIELD_LENGTH - length, file);
    }

    /* devmajor */
    sprintf(buffer, "%o", major(tree->sb.st_dev));
    fwrite(buffer, 1, DEVMAJOR_FIELD_LENGTH, file);

    /* devminor */
    sprintf(buffer, "%o", minor(tree->sb.st_dev));
    fwrite(buffer, 1, DEVMAJOR_FIELD_LENGTH, file);

    /* TODO: prefix */
    memset(buffer, 15, PREFIX_FIELD_LENGTH);
    fwrite(buffer, 1, PREFIX_FIELD_LENGTH, file);

    /* checksum calculation and write */
    fseek(file, -HEADER_LENGTH, SEEK_CUR);
    fread(buffer, 1, HEADER_LENGTH, file);
    for (checksum = 0, i = 0; i < HEADER_LENGTH; i++) {
        checksum += (unsigned char)buffer[i];
    }
    fseek(file, -(HEADER_LENGTH - CHKSUM_START_OFFSET), SEEK_CUR);
    sprintf(buffer, "%o", checksum);
    fwrite(buffer, 1, CHKSUM_FIELD_LENGTH, file);

    /* null padding of header block */
    fseek(file, HEADER_LENGTH - CHKSUM_START_OFFSET - CHKSUM_FIELD_LENGTH, SEEK_CUR);
    memset(buffer, 0, BLOCK_SIZE - HEADER_LENGTH);
    fwrite(buffer, 1, BLOCK_SIZE - HEADER_LENGTH, file);
}



