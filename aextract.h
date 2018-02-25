#ifndef aextract_h
#define aextract_h

FILE *extract_file(FILE *tarfile);

void find_archive(FILE *tarfile, char *paths[], int elements);


#endif
