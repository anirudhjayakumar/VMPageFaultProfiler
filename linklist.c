
#include <linux/kernel.h>    // included for KERN_INFO
#include <linux/init.h>      // included for __init and __exit macros
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/rwsem.h>

#include "linklist.h"
extern my_process_entry *entry_curr_task; 
my_process_entry proc_list;
static int list_size = 0;
struct rw_semaphore *sem = NULL;


int ll_initialize_list(void)
{
	INIT_LIST_HEAD(&proc_list.list);
	
	sem  = (struct rw_semaphore*)kmalloc(sizeof(struct rw_semaphore),GFP_KERNEL);
	init_rwsem(sem);

	return SUCCESS;
}

int ll_add_task(my_process_entry *new_proc)
{
	INIT_LIST_HEAD(&new_proc->list);
	down_write(sem);
	list_add_tail(&(new_proc->list),&(proc_list.list));
	up_write(sem);
	//printk(KERN_INFO "added pid=%d to list\n",pid);
	list_size++;

	return SUCCESS;
}

ulong ll_get_curr_utilization(void)
{   
	ulong util = 0;
	my_process_entry *proc_iter = NULL;
    down_read(sem);	
	list_for_each_entry(proc_iter,&proc_list.list,list) {
		util+= (proc_iter->computation*1000) / proc_iter->period;
	}
    up_read(sem);
	return util;
}

int ll_remove_task(pid_t pid)
{
	my_process_entry *proc_iter = NULL;
    down_write(sem);	
	list_for_each_entry(proc_iter,&proc_list.list,list) {
		if (proc_iter->pid == pid )
		{
			if(proc_iter == entry_curr_task) {
				 entry_curr_task = NULL;
			}
			del_timer(&(proc_iter->mytimer));
			list_del(&proc_iter->list);
			kfree(proc_iter);
			list_size--;
			up_write(sem);
			return SUCCESS;
		}
	}
	up_write(sem);
	return NOT_FOUND;
}

int ll_get_size(void)
{
	return list_size;
}

int ll_print_list(void)
{
	my_process_entry *proc_iter = NULL;
    down_read(sem);	
	list_for_each_entry(proc_iter,&proc_list.list,list) {
		printk(KERN_INFO "print: pid:%d period:%lu status:%d\n", proc_iter->pid, proc_iter->period, proc_iter->state); 
	}
    up_read(sem);
    return SUCCESS;
}

int ll_find_high_priority_task(my_process_entry **proc)
{
	
    my_process_entry *proc_iter = NULL;
ulong min_period = ULONG_MAX;
*proc = NULL;
    down_read(sem);	
	
	list_for_each_entry(proc_iter,&proc_list.list,list) {
		if(proc_iter->state == READY && proc_iter->period < min_period)
		{
			printk(KERN_INFO "high priority: pid:%d\n", proc_iter->pid); 
        		min_period = proc_iter->period;
			*proc = proc_iter;
		}
	}
    up_read(sem);
    if(*proc == NULL) return FAIL;
	else return SUCCESS;
}

int ll_cleanup(void)
{
	my_process_entry *proc_iter = NULL;
	//printk(KERN_INFO "linklist cleanup starts\n");
	down_write(sem);
	list_for_each_entry(proc_iter,&proc_list.list,list) {
		del_timer(&(proc_iter->mytimer));
		list_del(&proc_iter->list);
		kfree(proc_iter);
	}
	up_write(sem);
	kfree(sem);

	return SUCCESS;
}

int ll_get_task(pid_t pid, my_process_entry **proc)
{
my_process_entry *proc_iter = NULL;
	printk(KERN_INFO "ENTERED ll_get_task\n");
	*proc = NULL;
	
	printk(KERN_INFO  "BEFORE LOCK in ll_get_task\n");
    down_read(sem);	
	list_for_each_entry(proc_iter,&proc_list.list,list) {
		if(proc_iter->pid == pid ) {
			*proc = proc_iter;
		}
	}
    up_read(sem);
    if (*proc == NULL) return FAIL;
	else return SUCCESS;
}

int ll_generate_proc_info_string(char **buf, unsigned int *size)
{
	unsigned int count = 0;
	my_process_entry *proc_iter = NULL;
    *buf = (char *)kmalloc(BUF_SIZE,GFP_KERNEL);
	down_read(sem);	
	list_for_each_entry(proc_iter,&proc_list.list,list) {
		count += sprintf(*buf+count,"%d,%lu,%lu\n",
		proc_iter->pid,proc_iter->period,proc_iter->computation);
	}
    up_read(sem);
    (*buf)[count] = '\0';
    *size = count + 1;

	return SUCCESS;
}
