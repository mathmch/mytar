#ifndef aextract_h
#define aextract_h

void extract_file(FILE *tarfile, char *path);

void find_archives(FILE *tarfile, char *paths[], int elements);


#endif
