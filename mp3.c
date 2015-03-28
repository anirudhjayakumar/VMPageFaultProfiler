//////////////////////////////////////////////////
//	Author: Debjit Pal			//
//	Email: dpal2@illinois.edu		//
//////////////////////////////////////////////////

#include <linux/module.h>	/* Specifically for a module */
#include <linux/kernel.h>	/* We are doing kernel work after all */
#include <linux/timer.h>	/* For working with the timer */
#include <linux/proc_fs.h>	/* Necessary because we use proc file system*/
#include <linux/list.h>		/* Necessary because we will be using kernel linked list */
#include <asm/uaccess.h>	/* Necessary for the funtion copy_from_user and copy_to_user */
#include <linux/sched.h>	/* Necessary to get the access of linux kernel scheduler APIs 
				   And to get the pointer of type struct task_struct */
#include <linux/kthread.h>	
#include <linux/mutex.h>	/* Useful for accessing mutex lock APIs */
#include <linux/sysfs.h>	
#include <linux/jiffies.h>	/* Useful for jiffies counting */
#include <linux/types.h>	/* Useful to get the ssize_t, size_t, loff_t */
#include <linux/slab.h>		/* Useful for memory allocation using SLAB APIs */

#include "mp2_given.h"
#include "structure.h"		/* Defining the state enum and the process control block structures */
#include "linklist.h"
#include "thread.h"
// global variables
ulong initial_jiffies, curr_jiffies;
/* Pointer to the currently running task */
procfs_entry *newproc = NULL;
procfs_entry *newdir = NULL;
procfs_entry *newentry = NULL;
my_process_entry *entry_curr_task = NULL;
static void remove_entry(char *procname, char *parent);


/* 	Admission control makes sure that the new process which is trying to register can be accmodated
	given the present utilization of the CPU. If the new utilization is less than ln 2 or 0.693, 
	the new process gets admitted else its registration is denied.
*/

int admission_control (my_process_entry *new_process_entry) {
	ulong utilization = 0;
	/* Since floating point calculation is costly, we multiply by 1000 to make it an integer */
	utilization = (new_process_entry->computation)*1000 / (new_process_entry->period);
	utilization += ll_get_curr_utilization();
	printk(KERN_INFO "Utilization Sum: %lu", utilization);
	if(utilization <= 693) {
		return 0;
	}
	else {
		return -1;
	}
}


/* This is the upper half. It will change the process state to READY, reset the process timer and will wake up the bottom
   half which is the worker thread
 */

void mytimer_callback(ulong data) 
{
	my_process_entry *proc_entry = (my_process_entry*)data;
	// set the process to READY state
	proc_entry->state = READY;
	// wake up the dispatch thread 
	mod_timer(&(proc_entry->mytimer),jiffies + msecs_to_jiffies(proc_entry->period));
	wake_thread();
}

static ssize_t procfile_write(struct file *file, const char __user *buffer, size_t count, loff_t *data) {
	
	pid_t pid;
	char *proc_buffer;
	char *pid_str = NULL, *period_str = NULL, *computation_str = NULL, *end;
	my_process_entry *entry_temp;
	ulong ret = 0;

	/* Calling process passes the PID in the string format */
	proc_buffer = (char *)kmalloc(count + 1, GFP_KERNEL);
	if(copy_from_user(proc_buffer, buffer, count)) {
		kfree(proc_buffer);
		return -EFAULT;
	}
	/* Terminate with a NULL charecter to make it a C string */
	proc_buffer[count] = '\0';
	/* 	Handle differnt cases of the process through the proc file system
		R : Case for new process trying to register
		Y : Case for a process yielding
		D : Case for a process done with computing and going to deregister
	*/
	printk(KERN_INFO "proc_buffer = %s\n", proc_buffer);
	switch(proc_buffer[0]) {
		case 'R':

			/* Changing all commas to null terminated strings and storing them. */
			
			printk(KERN_INFO "Registering process\n");
			pid_str = proc_buffer + 2;
			end = strstr(proc_buffer + 2, ",");
			*end = '\0';
			period_str = end + 1;
			end = strstr(end + 1, ",");
			*end = '\0';
			computation_str = end + 1;
			printk(KERN_INFO "PROC_INFO:%s-%s-%s\n",pid_str,period_str,computation_str);
			
			/* Creating a temporary entry in kernel space to hold the new requesting process */
			entry_temp = (my_process_entry *)kmalloc(sizeof(my_process_entry), GFP_KERNEL);
			/*
				kstrtoul : Convert a string to an unsigned long
				int kstrtoul ( 	const char *s,
						unsigned int base,
						unsigned long *res);
				s: The start of the string and it must be null terminated.
				base:  The number base to use
				res: Where to write the result after conversion is over
			*/
			if((ret = kstrtoint(pid_str, 10,&(entry_temp->pid))) == -1) { 

				printk(KERN_ALERT "ERROR IN PID TO STRING CONVERSION\n");
				kfree(proc_buffer);
				kfree(entry_temp);
				return -EFAULT;
			}
			if((ret = kstrtoul(period_str, 10, &(entry_temp->period))) == -1) {
				printk(KERN_ALERT "ERROR IN PERIOD TO STRING CONVERSION\n");
				kfree(proc_buffer);
				kfree(entry_temp);
				return -EFAULT;
			}
			if((ret = kstrtoul(computation_str, 10, &(entry_temp->computation))) == -1) {
				printk(KERN_ALERT "ERROR IN COMPUTATION TO STRING CONVERSION\n");
				kfree(proc_buffer);
				kfree(entry_temp);
				return -EFAULT;
			}
		
			/* If every conversion success print the details in the kernel log for debugging purpose */
			curr_jiffies = jiffies;
			printk(KERN_INFO "RMS Scheduler receiving request from Process PID = %d, PERIOD =  %lu, COMPUTATION = %lu\n at %u us\n", entry_temp->pid, entry_temp->period, entry_temp->computation, jiffies_to_usecs(curr_jiffies));
		
			/* Call Admission Control function now to see if the new process can be registered */
			if(admission_control(entry_temp) == -1) {
				printk(KERN_ALERT "DENIED REGISTRATION OF PID = %d", entry_temp->pid);
				kfree(entry_temp);
				kfree(proc_buffer);
				return -EFAULT;
			}
		
			/* If we can register the process, then we have to check the time of registration */
			curr_jiffies = jiffies;
			printk(KERN_INFO "RMS Scheduler registering PROCESS PID = %d REGISTERED at %u us\n", entry_temp->pid, jiffies_to_usecs(curr_jiffies));

			/* We directly dont modify the process control block or PCB of the newly admitted process rather we keep a pointer 
			   to the PCB of the newly admitted process as suggested in the MP doc. We use the find_task_by_pid() function provided
			   in the mp2_given.h file for this purpose.
			*/
			entry_temp->task = find_task_by_pid(entry_temp->pid);
			if(!entry_temp->task)
			{
				printk(KERN_INFO "=== task retuened empty\n");
			}
			
			/* Once the pointer to PCB found, initialize the timer associated with the process 
			   See: http://www.ibm.com/developerworks/library/l-timers-list/
			   for setup_timer details
			*/

		        init_timer(&(entry_temp->mytimer));
			entry_temp->mytimer.expires = jiffies + msecs_to_jiffies(entry_temp->period);
			entry_temp->mytimer.data = (ulong)entry_temp;
			entry_temp->mytimer.function = mytimer_callback;
			add_timer(&(entry_temp->mytimer));

			//setup_timer(&(entry_temp->mytimer), &mytimer_callback, (ulong)entry_temp); 
			//casting pointer to ulong is a safe operation refer:http://www.makelinux.net/ldd3/chp-7-sect-4
	
			/* With an initialized timer, the user now needs to set the expiration time, which is done through a 
			   call to mod_timer. As users commonly provide an expiration in the future, 
			   they typically add jiffies here to offset from the current time.
			   See: http://www.ibm.com/developerworks/library/l-timers-list/
			   for mod_timer_details
			*/

			//ret = mod_timer(&(entry_temp->mytimer), jiffies + msecs_to_jiffies(entry_temp->period - entry_temp->computation));
			
			/* Finally, users can determine whether the timer is pending (yet to fire) through a 
			   call to timer_pending (1 is returned if the timer is pending):
			   See: http://www.ibm.com/developerworks/library/l-timers-list/
			   for timer_pending details
			*/
		
			if(ret) {
				printk(KERN_ALERT "ERROR IN SET TIMER\n");
				printk(KERN_ALERT "TIMER PENDING IS: %u\n", timer_pending(&(entry_temp->mytimer)));
			}
			entry_temp->state = SLEEPING; /* Set the process state to SLEEP and then add it to the process list */
			
			ll_add_task(entry_temp);
            

			break;
		case 'Y':
			/* Check if list is empty before yielding */
			if(ll_get_size() == 0) {
				printk(KERN_ALERT "PROCESS LIST IS EMPTY\n");
				kfree(proc_buffer);
				return -EFAULT;
			}
			
			printk(KERN_INFO "ENTERED case Y \n");
			pid_str = proc_buffer + 2;
			if((ret = kstrtoint(pid_str, 10, &pid)) == -1) {
				printk(KERN_ALERT "ERROR IN PID TO STRING CONVERSION\n");
				kfree(proc_buffer);
				return -EFAULT;
			}
			
			/* Find the entry associated with the process with PID which is trying to yield */
			
			ll_get_task(pid,&entry_temp);
			curr_jiffies = jiffies;
			printk(KERN_INFO "RMS Scheduler get the information Process PID: %d finished computation at %u us\n", entry_temp->pid, jiffies_to_usecs(curr_jiffies));

			printk(KERN_INFO "FROM YIELD SENT PID: %d LL_GET_TASK entry_temp->pid = %d", pid, entry_temp->pid);
		
			
			if(timer_pending(&entry_temp->mytimer)) {
				curr_jiffies = jiffies;
				printk(KERN_INFO "RMS Scheduler putting the Process PID: %d to sleep at %u us\n", entry_temp->pid, jiffies_to_usecs(curr_jiffies));
				set_task_state(entry_temp->task, TASK_UNINTERRUPTIBLE);
				entry_temp->sparam.sched_priority = 0;
				sched_setscheduler(entry_temp->task, SCHED_NORMAL, &(entry_temp->sparam));
				entry_temp->state = SLEEPING;
				entry_curr_task = NULL;
			}

			printk(KERN_INFO "PID %d is put to SLEEP.\n", entry_temp->pid);
			/* Our real time schedulling is done. Now call Linux scheduler to schedule everything else 
			   in the world
			*/
			schedule();
			printk(KERN_INFO "EXITING YIELD from KERNEL MODULE");
			break;

		case 'D':
			printk(KERN_INFO "ENTERING DE-REGISTRATION\n");
			/* Check if list is empty before deregistration */
			if(ll_get_size() == 0) {
				printk(KERN_ALERT "PROCESS LIST IS EMPTY\n");
				kfree(proc_buffer);
				return -EFAULT;
			}
			pid_str = proc_buffer + 2;
			printk(KERN_INFO "PID from D: %s\n", pid_str);
			if((ret = kstrtoint(pid_str, 10, &pid)) == -1) {
				printk(KERN_ALERT "ERROR IN PID TO STRING CONVERSION\n");
				kfree(proc_buffer);
				return -EFAULT;
			}

			curr_jiffies = jiffies;
			printk(KERN_INFO "RMS Scheduler removing Process PID: %d at %u us\n", pid, jiffies_to_usecs(curr_jiffies));
			if(ll_remove_task(pid) != SUCCESS) {
				printk(KERN_INFO "DEREGISTERING PROCESS: %d FAILED\n", pid);
				kfree(proc_buffer);
				return -EFAULT;
			}

			break;

		default:
			printk(KERN_ALERT "I DONT KNOW WHAT IS HAPPENING. PANICKED :(\n");
			kfree(proc_buffer);
			return -EFAULT;

	} /* End of the switch statement*/
	kfree(proc_buffer);
	return count;

}





/* Similar procfile_read function like MP1 */

static ssize_t procfile_read (struct file *file, char __user *buffer, size_t count, loff_t *data) {
	char *read_buf = NULL;
	unsigned int buf_size;
	ulong ret = 0;
	ssize_t len = count, retVal = 0;

	printk(KERN_INFO "PROCFILE READ /proc/mp2/status CALLED\n");
	ll_generate_proc_info_string(&read_buf,&buf_size);
	printk(KERN_INFO "*data = %d, buf_size = %d, count = %ld", (int)(*data), buf_size, count);
	if(*data >= buf_size) {
		kfree(read_buf);
		goto out;
	}

	if((int)(*data) + count > buf_size) {
		len = buf_size - (int)(*data);
	}


	if((ret = copy_to_user(buffer, read_buf,buf_size) != 0)) {
		printk(KERN_INFO "copy to user failed\n");
		return -EFAULT;
	}
	*data += (loff_t)(len - ret);
	retVal = len - ret;
	kfree(read_buf);
	out:
		return retVal;
}

static struct file_operations proc_file_op = {
	.owner 	= THIS_MODULE,
	.read	= procfile_read,
	.write	= procfile_write,
};

procfs_entry* proc_filesys_entries(char *procname, char *parent) {

	newdir = proc_mkdir(parent, NULL);
	if(newdir == NULL) {
		printk(KERN_ALERT "ERROR IN DIRECTORY CREATION\n");
	}
	else
		printk(KERN_ALERT "DIRECTORY CREATION SUCCESSFUL");
	
	newproc = proc_create(procname, 0666, newdir, &proc_file_op);
	if(newproc == NULL)
		printk(KERN_ALERT "ERROR: COULD NOT INITIALIZE /proc/%s/%s\n", parent, procname);

	printk(KERN_INFO "INFO: SUCCESSFULLY INITIALIZED /proc/%s/%s\n", parent, procname);
	
	return newproc;

}

static void remove_entry(char *procname, char *parent) {
	remove_proc_entry(procname, newdir);
	remove_proc_entry(parent, NULL);
}



static int __init mp2_init(void) {
	printk("MP2 MODULE LOADING");
	printk("MODULE INIT CALLED");
	newentry = proc_filesys_entries("status", "MP2");
    
	ll_initialize_list(); 
	thread_init();
	curr_jiffies = jiffies;
	printk(KERN_INFO "RMS Scheduler loaded at %u us\n", jiffies_to_usecs(curr_jiffies));
	printk("MP2 MODULE LOADED");
	return 0;
}

static void __exit mp2_exit(void) {
	printk("MP2 MODULE UNLOADING");
	remove_entry("status", "MP2");
	thread_cleanup();
	ll_cleanup();
	curr_jiffies = jiffies;
	printk(KERN_INFO "RMS Scheduler unloaded at %u us\n", jiffies_to_usecs(curr_jiffies));
	printk("MP2 MODULE UNLOADED");
}

module_init(mp2_init);
module_exit(mp2_exit);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Group_2");
MODULE_DESCRIPTION("CS-423_MP2");
