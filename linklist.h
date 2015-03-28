#ifndef LINKLIST_H
#define LINKLIST_H

#define SUCCESS 0
#define FAIL 1
#define NOT_FOUND 1001
#define BUF_SIZE 1024
#include "structure.h"



int ll_initialize_list(void);

int ll_add_task(my_process_entry *proc);

ulong ll_get_curr_utilization(void);

int ll_remove_task(pid_t pid);

int ll_get_size(void);

int ll_find_high_priority_task(my_process_entry **proc);

int ll_cleanup(void);

int ll_get_task(pid_t pid, my_process_entry **proc);

int ll_generate_proc_info_string(char **buf, unsigned int *size);

#endif
