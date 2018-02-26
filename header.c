#include "header.h"

#include <stdlib.h>
#include <string.h>

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
    fread(buffer, 1, MAGIC_LENGTH, tarfile);
    if (strncmp(buffer, MAGIC, magic_strlen) != 0)
        return -1; /* magic does not match */
    if (strict) {
        int version_strlen;
        if (buffer[magic_strlen] != '\0')
            return -1; /* missing null after magic */
        fread(buffer, 1, VERSION_LENGTH, tarfile);
        version_strlen = strlen(VERSION);
        if (strncmp(buffer, VERSION, version_strlen) != 0)
            return -1; /* version does not match */
        fseek(tarfile, -VERSION_LENGTH, SEEK_CUR);
    }

    /* validate the checksum */
    fseek(tarfile, -MAGIC_LENGTH - MAGIC_OFFSET + CHKSUM_OFFSET, SEEK_CUR);
    fread(buffer, 1, CHKSUM_LENGTH, tarfile);
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
        if (position > CHKSUM_OFFSET && position < CHKSUM_OFFSET + CHKSUM_LENGTH)
            checksum += (int)(' ');
        else
            checksum += c;
    }
    fseek(tarfile, -HEADER_LENGTH, SEEK_CUR);
    return checksum;
}
