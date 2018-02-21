#ifndef dirnode_h
#define dirnode_h

#include <sys/stat.h>

#define PATH_MAX 256

typedef struct dirnode dirnode;
struct dirnode {
    char path_name[PATH_MAX];
    struct stat sb;
    struct dirnode **children;
    int child_count;
};

dirnode *build_tree(char *path, char *prefix);

void print_tree(dirnode *tree);

#endif /* dirnode_h */
