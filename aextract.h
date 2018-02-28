#ifndef aextract_h
#define aextract_h

void extract_file(FILE *tarfile, char *path, int isverbose, int isstrict);

void find_archives(FILE *tarfile, char *paths[], int elements,
		   int isverbose, int isstrict);


#endif
