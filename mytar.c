#include "dirnode.h"
#include <stdlib.h>

int main(int argc, char *argv[]) {
    dirnode *tree = build_tree(argv[1]);
    char *path_array[30]; /*temp array size for tests */
    print_tree(tree, path_array, 0);
    return 0;
}
