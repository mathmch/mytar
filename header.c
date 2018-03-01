#include "header.h"

#include <stdlib.h>
#include <string.h>
#include "util.h"

/* assuming at the start of the header, validate it by checking
 * magic number (+ null if strict), version (if strict), and checksum.
 * returns 0 for a valid header and -1 for an invalid header. */
int validate_header(FILE *tarfile, int strict) {
    char buffer[CHKSUM_LENGTH];
    int magic_strlen = (int)strlen(MAGIC);
    int checksum;
    int actual_checksum;
    memset(buffer, 0, CHKSUM_LENGTH);
    fseek(tarfile, MAGIC_OFFSET, SEEK_CUR);
    safe_fread(buffer, 1, MAGIC_LENGTH, tarfile);
    if (strncmp(buffer, MAGIC, magic_strlen) != 0)
        return -1; /* magic does not match */
    if (strict) {
        int version_strlen;
        if (buffer[magic_strlen] != '\0')
            return -1; /* missing null after magic */
        safe_fread(buffer, 1, VERSION_LENGTH, tarfile);
        version_strlen = strlen(VERSION);
        if (strncmp(buffer, VERSION, version_strlen) != 0)
            return -1; /* version does not match */
        fseek(tarfile, -VERSION_LENGTH, SEEK_CUR);
    }

    /* validate the checksum */
    fseek(tarfile, -MAGIC_LENGTH - MAGIC_OFFSET + CHKSUM_OFFSET, SEEK_CUR);
    safe_fread(buffer, 1, CHKSUM_LENGTH, tarfile);
    checksum = (int)strtol(buffer, NULL, 8);
    fseek(tarfile, -CHKSUM_LENGTH - CHKSUM_OFFSET, SEEK_CUR);
    actual_checksum = compute_checksum(tarfile);
    return checksum == actual_checksum;
}

/* assuming at the start of the header, compute its checksum.
 * for this computation, the checksum field is treated as spaces. */
int compute_checksum(FILE *tarfile) {
    int checksum = 0;
    int position;
    for (position = 0; position < HEADER_LENGTH; position++) {
        int c = getc(tarfile);
        if (position > CHKSUM_OFFSET &&
	    position < CHKSUM_OFFSET + CHKSUM_LENGTH)
            checksum += (int)(' ');
        else
            checksum += c;
    }
    fseek(tarfile, -HEADER_LENGTH, SEEK_CUR);
    return checksum;
}

/* assume at start of header, resets to start of header */
int is_dir(FILE *tarfile) {
    char buffer[NAME_LENGTH + 1];
    safe_fread(buffer, 1, NAME_LENGTH, tarfile);
    buffer[NAME_LENGTH] = '\0';
    fseek(tarfile, -NAME_LENGTH, SEEK_CUR);
    return buffer[strlen(buffer) - 1] == '/';
}

/* assume at start of header, resets to start of header */
int is_symlink(FILE *tarfile) {
    char linkname_start;
    fseek(tarfile, LINKNAME_OFFSET, SEEK_CUR);
    safe_fread(&linkname_start, 1, 1, tarfile);
    fseek(tarfile, -LINKNAME_OFFSET - 1, SEEK_CUR);
    return linkname_start != '\0';
}

/* assume at start of header, resets to start of header */
int is_reg(FILE *tarfile) {
    return !(is_dir(tarfile) || is_symlink(tarfile));
}

/* assume at start of header, resets to start of header */
void get_path(char buffer[], FILE *tarfile) {
    char name[NAME_LENGTH + 1];
    fseek(tarfile, PREFIX_OFFSET, SEEK_CUR);
    safe_fread(buffer, 1, PREFIX_LENGTH, tarfile);
    buffer[PREFIX_LENGTH + 1] = '\0';
    if(buffer[0] != '\0' && buffer[strlen(buffer) - 1] != '/')
        strcat(buffer, "/");
    fseek(tarfile, -PREFIX_OFFSET - PREFIX_LENGTH, SEEK_CUR);
    safe_fread(name, 1, NAME_LENGTH, tarfile);
    name[NAME_LENGTH] = '\0';
    strcat(buffer, name);
    fseek(tarfile, -NAME_LENGTH, SEEK_CUR);
}

/* assume at start of header, resets to start of header */
mode_t get_mode(FILE *tarfile) {
    char buffer[MODE_LENGTH];
    fseek(tarfile, MODE_OFFSET, SEEK_CUR);
    safe_fread(buffer, 1, MODE_LENGTH, tarfile);
    fseek(tarfile, -MODE_OFFSET - MODE_LENGTH, SEEK_CUR);
    return strtol(buffer, NULL, 8);
}

/* assume at start of header, resets to start of header */
off_t get_size(FILE *tarfile) {
    char buffer[SIZE_LENGTH];
    fseek(tarfile, SIZE_OFFSET, SEEK_CUR);
    safe_fread(buffer, 1, SIZE_LENGTH, tarfile);
    fseek(tarfile, -SIZE_OFFSET - SIZE_LENGTH, SEEK_CUR);
    return strtol(buffer, NULL, 8);
}

/* assume at start of header, resets to start of header */
time_t get_mtime(FILE *tarfile) {
    char buffer[MTIME_LENGTH];
    fseek(tarfile, MTIME_OFFSET, SEEK_CUR);
    safe_fread(buffer, 1, MTIME_LENGTH, tarfile);
    fseek(tarfile, -MTIME_LENGTH - MTIME_OFFSET, SEEK_CUR);
    return strtol(buffer, NULL, 8);
}

/* assume at start of header, resets to start of header */
void get_linkname(char buffer[], FILE *tarfile) {
    fseek(tarfile, LINKNAME_OFFSET, SEEK_CUR);
    safe_fread(buffer, 1, LINKNAME_LENGTH, tarfile);
    fseek(tarfile, -LINKNAME_LENGTH - LINKNAME_OFFSET, SEEK_CUR);
}

int size_to_blocks(off_t size) {
    if (size % BLOCK_LENGTH == 0)
        return (int)(size / BLOCK_LENGTH);
    else
        return (int)((size / BLOCK_LENGTH) + 1);
}
