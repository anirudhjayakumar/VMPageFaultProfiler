
#ifndef MEM_H
#define MEM_H
#include "structure.h"

#define MMAP_BUF_SIZE (128 * PAGE_SIZE)
#define MAX_SAMPLES 12000

int mm_initialize(void);
int mm_add_data(sampling_data *data);
void* mm_get_buffer(void);
int mm_cleanup(void);
#endif //MEM_H
