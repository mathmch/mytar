#ifndef dirnode_h
#define dirnode_h

#include <sys/stat.h>

typedef struct dirnode dirnode;
struct dirnode {
    char *name;
    struct stat sb;
    struct dirnode **children;
    int child_count;
};

dirnode *build_tree(char *path);

void print_tree(dirnode *tree, char *path);

#endif /* dirnode_h */
