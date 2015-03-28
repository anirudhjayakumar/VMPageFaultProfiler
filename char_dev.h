#ifndef CHAR_DEV_H
#define CHAR_DEV_H
#include "structure.h"
// function signatures are incomplete
int cd_initialize(void);
int cd_cleanup(void);
int open(void);
int close(void);
int mmap(void);

#endif //CHAR_DEV_H
