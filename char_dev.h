#ifndef CHAR_DEV_H
#define CHAR_DEV_H
#include "structure.h"
// function signatures are incomplete
int cd_initialize();
int cd_cleanup();
int open();
int close();
int mmap();

#endif //CHAR_DEV_H
