#ifndef acreate_h
#define acreate_h

void archive(char *file_name, dirnode *tree);

void archive_helper(FILE *file, dirnode *tree);

void write_header(FILE *file, dirnode *tree);

int split_path(char *path, char *prefix, char *name);

void write_contents(FILE *file, dirnode *tree);

#endif 
