#ifndef dirnode_h
#define dirnode_h

#include <sys/stat.h>
#include "header.h"

typedef struct dirnode dirnode;
struct dirnode {
    char path_name[PATH_MAX];
    struct stat sb;
    struct dirnode **children;
    int child_count;
};

dirnode *build_tree(char *path, char *prefix);

void print_tree(dirnode *tree);

void free_tree(dirnode *tree);

#endif
