#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "header.h"

/* malloc or exit with the message upon failure */
void *safe_malloc(size_t size, const char *msg) {
    void *new = malloc(size);
    if (new == NULL) {
        perror(msg);
        exit(EXIT_FAILURE);
    }

    return new;
}

/* calloc or exit with the message upon failure */
void *safe_calloc(size_t count, size_t size, const char *msg) {
    void *new = calloc(count, size);
    if (new == NULL) {
        perror(msg);
        exit(EXIT_FAILURE);
    }

    return new;
}

/* realloc or exit with the message upon failure */
void *safe_realloc(void *ptr, size_t size, const char *msg) {
    void *new = realloc(ptr, size);
    if (new == NULL) {
        perror(msg);
        exit(EXIT_FAILURE);
    }

    return new;
}

/* fread or exit with message on failure */
size_t safe_fread(char buffer[], int size, int nmemb, FILE *file) {
    size_t bytes_read = fread(buffer, size, nmemb, file);
    if (bytes_read == 0 && ferror(file)) {
        perror("fread");
        exit(EXIT_FAILURE);
    }
    return bytes_read;
}

size_t safe_fwrite(char buffer[], int size, int nmemb, FILE *file) {
    size_t bytes_written = fwrite(buffer, size, nmemb, file);
    if (bytes_written == 0 && ferror(file)) {
        perror("fwrite");
        exit(EXIT_FAILURE);
    }
    return bytes_written;
}

/* returns a newly malloced copy of the string */
char *new_copy(char *str) {
    unsigned long length;
    char *copy;
    if (str == NULL)
        return NULL;
    length = strlen(str);
    copy = safe_malloc(length, "new_copy");
    strcpy(copy, str);
    return copy;
}

/* returns a new string from concatenating the three strings.
 * if a string is null, it is ignored in the copy string.
 * the result of this function must 
 * be freed later if it does not return null. */
char *concat(char *str1, char *str2, char *str3) {
    /* could use varargs here, but eh... */
    #define STR_COUNT 3
    char *strings[STR_COUNT];
    unsigned long lengths[STR_COUNT];
    unsigned long total_length = 0;
    char *new, *current;
    int i = 0;

    strings[i++] = str1;
    strings[i++] = str2;
    strings[i] = str3;
    
    for (i = 0; i < STR_COUNT; i++) {
        unsigned long length = (strings[i] == NULL) ? 0 : strlen(strings[i]);
        lengths[i] = length;
        total_length += length;
    }

    new = safe_malloc(total_length + 1, "concat");
    current = new;
    for (i = 0; i < STR_COUNT; i++) {
        if (strings[i] == NULL)
            continue;
        strcpy(current, strings[i]);
        current += lengths[i];
    }

    new[total_length] = '\0';
    return new;
}

/* count the occurrences of a character in the string */
int count_occur(char *path, char c) {
    int i;
    int count;
    if (path == NULL)
        return 0;
    count = 0;
    for (i = 0; path[i] != '\0'; i++) {
        if (path[i] == c)
            count++;
    }
    return count;
}
