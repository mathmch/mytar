#ifndef header_h
#define header_h

void find_listings(FILE *tarfile, char *paths[],
		   int elements, int isverbose, int isstrict);

void list_contents(FILE* tarfile, char path[], int isverbose, int isstrict);

void get_permissions(char permissions[], mode_t mode, FILE *tarfile);

void get_owner(uid_t uid, char uname[], gid_t gid, char gname[], char owner[]);

void get_time(time_t time, char timestr[]);

void find_listings(FILE *tarfile, char *paths[],
		   int elements, int isverbose, int isstrict);


#endif
