#ifndef STRUCTS_H
#define STRUCTS_H

struct dirnode{
  char *name;
  struct **dirnode children;
  struct stat sb;
}

#endif
