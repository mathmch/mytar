#ifndef header_h
#define header_h

#include <stdio.h>

#define BLOCK_LENGTH 512
#define MAGIC "ustar"
#define VERSION "00"

/* header field lengths */
#define NAME_LENGTH 100
#define MODE_LENGTH 8
#define UID_LENGTH 8
#define GID_LENGTH 8
#define SIZE_LENGTH 12
#define MTIME_LENGTH 12
#define CHKSUM_LENGTH 8
#define TYPEFLAG_LENGTH 1
#define LINKNAME_LENGTH 100
#define MAGIC_LENGTH 6
#define VERSION_LENGTH 2
#define UNAME_LENGTH 32
#define GNAME_LENGTH 32
#define DEVMAJOR_LENGTH 8
#define DEVMINOR_LENGTH 8
#define PREFIX_LENGTH 155
#define HEADER_LENGTH 500

/* header field start offsets */
#define NAME_OFFSET 0
#define MODE_OFFSET (NAME_OFFSET + NAME_LENGTH)                 /* 100 */
#define UID_OFFSET (MODE_OFFSET + MODE_LENGTH)                  /* 108 */
#define GID_OFFSET (UID_OFFSET + UID_LENGTH)                    /* 116 */
#define SIZE_OFFSET (GID_OFFSET + GID_LENGTH)                   /* 124 */
#define MTIME_OFFSET (SIZE_OFFSET + SIZE_LENGTH)                /* 136 */
#define CHKSUM_OFFSET (MTIME_OFFSET + MTIME_LENGTH)             /* 148 */
#define TYPEFLAG_OFFSET (CHKSUM_OFFSET + CHKSUM_LENGTH)         /* 156 */
#define LINKNAME_OFFSET (TYPEFLAG_OFFSET + TYPEFLAG_LENGTH)     /* 157 */
#define MAGIC_OFFSET (LINKNAME_OFFSET + LINKNAME_LENGTH)        /* 257 */
#define VERSION_OFFSET (MAGIC_OFFSET + MAGIC_LENGTH)            /* 263 */
#define UNAME_OFFSET (VERSION_OFFSET + VERSION_LENGTH)          /* 265 */
#define GNAME_OFFSET (UNAME_OFFSET + UNAME_LENGTH)              /* 297 */
#define DEVMAJOR_OFFSET (GNAME_OFFSET + GNAME_LENGTH)           /* 329 */
#define DEVMINOR_OFFSET (DEVMAJOR_OFFSET + DEVMAJOR_LENGTH)     /* 337 */
#define PREFIX_OFFSET (DEVMINOR_OFFSET + DEVMINOR_LENGTH)       /* 345 */

int validate_header(FILE *tarfile, int strict);

int compute_checksum(FILE *tarfile);

#endif
