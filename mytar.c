#include "dirnode.h"

int main(int argc, char *argv[]) {
    dirnode *tree = build_tree(argv[1]);
    print_tree(tree, NULL);
    return 0;
}
