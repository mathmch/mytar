#include "dirnode.h"
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include "util.h"

dirnode *build_tree(char *path) {
    #define FN_NAME "build_tree"
    struct stat sb;
    dirnode *tree = safe_malloc(sizeof(struct dirnode), FN_NAME);
  
    if (lstat(path, &sb)) {
        perror(path);
        exit(EXIT_FAILURE);
    }

    tree->name = path;
    tree->sb = sb;
    tree->children = NULL;
    tree->child_count = 0;

    if (S_ISDIR(sb.st_mode)) {
        DIR *dir = opendir(path);
        struct dirent *entry;
        chdir(path);
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;
            tree->child_count++;
            tree->children = safe_realloc(tree->children, tree->child_count * sizeof(dirnode*), FN_NAME);
            tree->children[tree->child_count - 1] = build_tree(entry->d_name);
        }
        chdir("..");
    }

    return tree;
}

/* only problem is this function puts / at the end of every path, even if
   final spot is a file, not a dir. */
void print_tree(dirnode *tree, char *path_array[], int depth) {
    char *new_path;
    int length;
    int i;
  
    path_array[depth++] = tree->name;
    for(i = 0; i < depth; i++){
	length = strlen(path_array[i]);
	fwrite(path_array[i], length, 1, stdout);
	printf("/");
    }
    printf("\n");
    for (i = 0; i < tree->child_count; i++) {
	print_tree(tree->children[i], path_array, depth);
    }
    
}
