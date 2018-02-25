#ifndef aextract_h
#define aextract_h

FILE *extract_file(FILE *tarfile, char *path);

void find_archive(FILE *tarfile, char *paths[], int elements);


#endif
