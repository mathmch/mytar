#ifndef util_h
#define util_h

void *safe_malloc(size_t size, const char *msg);

void *safe_calloc(size_t count, size_t size, const char *msg);

void *safe_realloc(void *ptr, size_t size, const char *msg);

char *concat(char *str1, char *str2, char *str3);

#endif
