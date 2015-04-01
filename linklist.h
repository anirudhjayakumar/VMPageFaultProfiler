/* this file contains the interfaces to the linklist module 
   The functions in the file are all thread-safe. They use 
   read-write semaphores
*/

#define DUPLICATE 1000 // indicate duplicate entry in the list
                       // starts from 1000 to avoid other return
					   // codes in the kernel.
#define NOT_FOUND 1001					   
#include "structure.h"
/* initilalize_list() to be used before using this module. Ideally,
this should be called from the module init function. */
int ll_initialize_list(void);



/*
add_to_list(int pid) takes in the pid of the new process.

return SUCCESS if the pid is added successfully to the list
return DUPLICATE if pid is already present in the list
return KERNEL return codes in case of other errors

*/
int ll_add_task(struct process_info *new_proc);
/*
Gives a list of pids with the cpu times. This is called when the 
user-space tries to read a the status entry

buf - char array
count - totol size of data

delete buffer by the caller
*/
int ll_generate_proc_info_string(char **buf,int *count);

/*
Update list: updates the pid with the new cpu usage(
*/
int ll_update_item(int pid,ulong min_flt_,ulong maj_flt_,ulong cpu_util_,ulong last_jiff);

/*
cleanup(): frees all allocated memory and empties the list
*/

int ll_cleanup(void);

/* is_pid_in_list(int pid) see if pid is present in the list.
*/
int ll_is_pid_in_list(int pid);

/* get_pids() returns an array of pids with the pid count 
   caller function should free the memory after use
*/
int ll_get_pids(int **pids, int *count);

/* delete pid from list */
int ll_delete_item(int pid);

/* print the contents of the list to kernel logs */
void ll_print_list(void);

int ll_list_size(void);

int ll_get_last_sample_jiff(int pid, ulong *prev_jiff);
