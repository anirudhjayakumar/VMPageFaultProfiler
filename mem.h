
#ifndef MEM_H
#define MEM_H
#include "structure.h"
int mm_initialize(void);
int mm_add_data(sampling_data *data);
void* mm_get_buffer(void);
int mm_cleanup(void);
#endif //MEM_H
