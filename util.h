#ifndef util_h
#define util_h

void *safe_malloc(size_t size, const char *msg);

void *safe_calloc(size_t count, size_t size, const char *msg);

void *safe_realloc(void *ptr, size_t size, const char *msg);

char *concat(char *str1, char *str2, char *str3);

int size_to_blocks(off_t size);

void get_path(char buffer[], FILE *tarfile);

mode_t get_mode(FILE *tarfile);

off_t get_size(FILE *tarfile);
#endif /* util_h */
