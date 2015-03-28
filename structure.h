#ifndef STRUCTURE_H
#define STRUCTURE_H

#include <linux/types.h>
#include <linux/sched.h>
enum States {
	SLEEPING,	// 0
	READY,		// 1
	RUNNING,	// 2
};

typedef unsigned long ulong;


typedef struct process_entry {
	/* Data Structure elements */		/* Data Strcuture element explnation */
	int pid;				// Process ID
	ulong period;				// Period of the process
	ulong computation;			// Computation time of process
	ulong c;                // Question: What is this for?
	enum States state;			// States of operation of the process coming from enum
	struct sched_param sparam;		// Scheduling parameter of the process
	struct timer_list mytimer;		// Timer for wakingup
	struct task_struct *task;		// Linux task pointer
	struct list_head list;
} my_process_entry;

typedef struct proc_dir_entry procfs_entry;

procfs_entry* proc_filesys_entries(char *procname, char *parent);

/* Admission Control Function Declaration */
int admission_control (my_process_entry *new_process_entry);
/* Remove task function declaration used when a process is deregistering */
int remove_task(pid_t pid);
/* mytimer call back interface */
void mytimer_callback(ulong data);
/* Worker thread to preempt the lower priority process and to execute the higher priority task */
int workthread(void *data);

#endif // STRUCTURE_H
