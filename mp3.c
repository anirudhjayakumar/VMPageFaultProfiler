/////////////////////////////////////////////////
//	Author: Debjit Pal			//
//	Email: dpal2@illinois.edu		//
//////////////////////////////////////////////////

#include <linux/module.h>	/* Specifically for a module */
#include <linux/kernel.h>	/* We are doing kernel work after all */
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

#include "mp3_given.h"
#include "structure.h"		/* Defining the state enum and the process control block structures */
#include "workqueue.h"
#include "linklist.h"
#include <linux/pid.h>
#include "mem.h"
#include "char_dev.h"

/* Pointer to the currently running task */
procfs_entry *newproc = NULL;
procfs_entry *newdir = NULL;
procfs_entry *newentry = NULL;
static void remove_entry(char *procname, char *parent);

struct task_struct* find_task_by_pid(unsigned int nr)
{
    struct task_struct* task = NULL;
    rcu_read_lock();
    task=pid_task(find_vpid(nr), PIDTYPE_PID);
    rcu_read_unlock();
    if (task == NULL)
        printk(KERN_INFO "find_task_by_pid: couldnt find pid %d\n", nr);
    return task;
}



int get_cpu_use(int pid, unsigned long *min_flt, unsigned long *maj_flt,
         unsigned long *utime, unsigned long *stime)
{
        int ret = -1;
        struct task_struct* task;
        rcu_read_lock();
        task=find_task_by_pid(pid);
        if (task!=NULL) {
                *min_flt=task->min_flt;
                *maj_flt=task->maj_flt;
                *utime=task->utime;
                *stime=task->stime;

                task->maj_flt = 0;
                task->min_flt = 0;
                task->utime = 0;
                task->stime = 0;
                ret = 0;
        }
        rcu_read_unlock();
        return ret;
}




static ssize_t procfile_write(struct file *file, const char __user *buffer, size_t count, loff_t *data) {
	
	pid_t pid;
	char *proc_buffer;
	char *pid_str = NULL;
	struct process_info *entry_temp;
	ulong ret = 0;
    printk (KERN_INFO "Entering procfile_write()\n");
	/* Calling process passes the PID in the string format */
	proc_buffer = (char *)kmalloc(count + 1, GFP_KERNEL);
	if(copy_from_user(proc_buffer, buffer, count)) {
		kfree(proc_buffer);
		printk(KERN_INFO "copy_from_user() failed\n");
		return -EFAULT;
	}
	/* Terminate with a NULL charecter to make it a C string */
	proc_buffer[count] = '\0';
	/* 	Handle differnt cases of the process through the proc file system
		R : Case for new process trying to register
		D : Case for a process done with computing and going to deregister
	*/
	printk(KERN_INFO "proc_buffer = %s\n", proc_buffer);
	switch(proc_buffer[0]) {
		case 'R':

			/* Changing all commas to null terminated strings and storing them. */
			
			printk(KERN_INFO "Registering process\n");
			pid_str = proc_buffer + 2;
			printk(KERN_INFO "PROC_INFO:%s\n",pid_str);
			
			/* Creating a temporary entry in kernel space to hold the new requesting process */
			entry_temp = (struct process_info *)kmalloc(sizeof(struct process_info), GFP_KERNEL);
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
			
		
			/* We directly dont modify the process control block or PCB of the newly admitted process rather we keep a pointer 
			   to the PCB of the newly admitted process as suggested in the MP doc. We use the find_task_by_pid() function provided
			   in the mp3_given.h file for this purpose.
			*/
			entry_temp->task = find_task_by_pid(entry_temp->pid);
            entry_temp->min_flt = 0;
            entry_temp->maj_flt = 0;
            entry_temp->cpu_util = 0;
			entry_temp->start_jiff = entry_temp->last_sample_jiff = jiffies; 
			//useful entry to calculate cpu util
			if(!entry_temp->task)
			{
				printk(KERN_INFO "=== task retuened empty\n");
			}
			
			ll_add_task(entry_temp);
			if(ll_list_size() == 1) {
				printk(KERN_INFO "Added first process. Now creating workqueue\n");
				wq_create_workqueue(); //creates work and  workqueus
				wq_modify_timer(); // starttimer
			}
			
			break;
		

		case 'U':
			printk(KERN_INFO "ENTERING DE-REGISTRATION\n");
			/* Check if list is empty before deregistration */
			if(ll_list_size() == 0) {
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

			if(ll_delete_item(pid) != SUCCESS) {
				printk(KERN_INFO "DEREGISTERING PROCESS: %d FAILED\n", pid);
				kfree(proc_buffer);
			}
			
			if(ll_list_size() == 0){
				wq_destroy_workqueue();
                mm_set_mem_index();    
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

	printk(KERN_INFO "PROCFILE READ /proc/mp3/status CALLED\n");
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



static int __init mp3_init(void) {
	printk("MP3 MODULE LOADING");
	printk("MODULE INIT CALLED");
	newentry = proc_filesys_entries("status", "mp3"); 
	printk("PAGE SIZE = %lu\n",PAGE_SIZE);
    
	ll_initialize_list();
	wq_init_workqueue();
    	mm_initialize();
	cd_initialize();
	printk("MP3 MODULE LOADED");
	return 0;
}

static void __exit mp3_exit(void) {
	printk("MP3 MODULE UNLOADING");
	remove_entry("status", "mp3");
	mm_cleanup();
	wq_cleanup_workqueue();
    ll_cleanup();
	cd_cleanup();
	printk("MP3 MODULE UNLOADED");
}

module_init(mp3_init);
module_exit(mp3_exit);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Group_2");
MODULE_DESCRIPTION("CS-423_MP3");
