#ifndef STRUCTURE_H
#define STRUCTURE_H

#include <linux/types.h>
#include <linux/sched.h>

#define SUCCESS 0
#define FAIL    1
#define TRUE    1
#define FALSE   0

typedef unsigned long ulong;

typedef struct sample {
    ulong jiff_value;
	ulong min_flt;
	ulong maj_flt;
    ulong cpu_util;
} sampling_data;

struct process_info {
	/* Data Structure elements */		/* Data Strcuture element explnation */
	int pid;				// Process ID
	struct task_struct *task;		// Linux task pointer
	struct list_head list;
	ulong start_jiff;
	ulong last_sample_jiff;
	ulong min_flt;
	ulong maj_flt;
	ulong cpu_util; // utime + stime
};

typedef struct proc_dir_entry procfs_entry;

procfs_entry* proc_filesys_entries(char *procname, char *parent);


/*dd Admission Control Function Declaration */
//int admission_control (my_process_entry *new_process_entry);
/* Remove task function declaration used when a process is deregistering */
//int remove_task(pid_t pid);
/* mytimer call back interface */
//void mytimer_callback(ulong data);
/* Worker thread to preempt the lower priority process and to execute the higher priority task */
//int workthread(void *data);

#endif // STRUCTURE_H
