#include "dirnode.h"
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include "util.h"

/* build the directory tree from the path */
dirnode *build_tree(char *path, char *prefix) {
    #define FN_NAME "build_tree"
    struct stat sb;
    char *new_path;
    dirnode *tree = safe_malloc(sizeof(struct dirnode), FN_NAME);

    if (lstat(path, &sb)) {
        perror(path);
        return NULL;
    }

    if (prefix == NULL) {
        /* dup so we can be confident when we free later */
        new_path = strdup(path);
    } else if (prefix[strlen(prefix) - 1] != '/') {
        /* (concat requires freeing too) */
        new_path = concat(prefix, "/", path);
    } else {
        new_path = concat(prefix, path, NULL);
    }

    strcpy(tree->path_name, new_path);
    tree->sb = sb;
    tree->children = NULL;
    tree->child_count = 0;

    if (S_ISDIR(sb.st_mode)) {
        DIR *dir = opendir(path);
        struct dirent *entry;
        chdir(path);
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0
                || strcmp(entry->d_name, "..") == 0)
                continue;
            tree->child_count++;
            tree->children =safe_realloc(tree->children,
                                         tree->child_count * sizeof(dirnode*),
                                         FN_NAME);
            tree->children[tree->child_count-1] = build_tree(entry->d_name,
                                                             new_path);
        }

        /* go back up after recursing over all children */
        chdir("..");

        /* a directory has 0 size */
        tree->sb.st_size = 0;

        /* directory, so add a slash to the path name end after recursing
         (if it's not already there) */
        if (tree->path_name[strlen(tree->path_name) - 1] != '/')
            strcat(tree->path_name, "/");
    }

    free(new_path);
    return tree;
}

/* recursively print the file names in the tree */
void print_tree(dirnode *tree) {
    int i;
    if (!S_ISREG(tree->sb.st_mode) &&
        !S_ISDIR(tree->sb.st_mode) && !S_ISLNK(tree->sb.st_mode)) {
        /* unsupported file type, does not go in the archive */
        return;
    }
    puts(tree->path_name);
    for (i = 0; i < tree->child_count; i++) {
        print_tree(tree->children[i]);
    }
}

/* recursively free the tree */
void free_tree(dirnode *tree) {
    int i;
    for (i = 0; i < tree->child_count; i++)
        free_tree(tree->children[i]);
    free(tree);
}
