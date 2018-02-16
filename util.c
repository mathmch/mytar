#include <stdio.h>
#include <stdlib.h>

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
