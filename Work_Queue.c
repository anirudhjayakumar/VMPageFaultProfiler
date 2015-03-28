
#include <linux/slab.h> // for kmalloc
#include <linux/workqueue.h> // workqueue_struct
#include<linux/timer.h>
#include<linux/jiffies.h>
#include<linux/mutex.h>
#include <linux/pid.h>
#include "common.h"
#include "linklist.h"
#include <linux/sched.h>
#include <linux/fs_struct.h>
#define find_task_by_pid(nr) pid_task(find_vpid(nr), PIDTYPE_PID)
static struct timer_list intr_timer;

/*Work Queue:- my_wq*/
static struct workqueue_struct *my_wq;
//creating work element:- work
struct work_struct *work;

void modify_timer(void)
{
	mod_timer(&intr_timer, jiffies+5*HZ);	
}
//THIS FUNCTION RETURNS 0 IF THE PID IS VALID AND THE CPU TIME IS SUCCESFULLY RETURNED BY THE PARAMETER CPU_USE. OTHERWISE IT RETURNS -1
int get_cpu_use(int pid, unsigned long *cpu_use)
{
   struct task_struct* task;
   rcu_read_lock();
   task=find_task_by_pid(pid);
   if (task!=NULL)
   {  
	*cpu_use=task->utime;
        rcu_read_unlock();
        return SUCCESS;
   }
   else 
   {
     rcu_read_unlock();
     return FAIL;
   }
}


//Work handler to update CPU time in Linked List
void work_handler( struct work_struct *work )
{
	int *pids = NULL;
    int count = 0;
   	int index,pid;
    unsigned long cpu_time;
//	Works *wk = (Works*)work;
	/*Insert code here for updation of CPU Time*/
    // get the pids
	//printk(KERN_INFO "work handler called\n");
	ll_get_pids(&pids,&count);
	//printk(KERN_INFO "PID count = %d", count);
    for (index = 0; index <count; ++index)
	{

		pid = pids[index];

		if( get_cpu_use(pid,&cpu_time) == SUCCESS )
		{
			//printk(KERN_INFO "PID: %d CPU TIME: %lu\n", pid,cpu_time);
			ll_update_time(pid,jiffies_to_msecs(cputime_to_jiffies(cpu_time))); 
		}
		else
		{ 
			//printk(KERN_INFO "get_cpu_use() failed");
			ll_delete_pid(pid);
		}
	}
	
	kfree(pids);
}

/** function to create workqueue*/
void create_work_queue(void)
{
	//printk(KERN_INFO "workqueue creation called\n");
	my_wq= create_workqueue( "mp1_workqueue\n" );
	if ( !my_wq ) {
		//printk( "Error!Workqueue could not be created\n" );
		return ;
	}
	work = (struct work_struct*)kmalloc(sizeof(struct work_struct), GFP_KERNEL );
	if ( work ) {
		
	    //printk(KERN_INFO "INIT_WORK\n");
		INIT_WORK( work, work_handler );
		//wk1->number = 1;
	}
}

void timer_callback(unsigned long data){
	//printk(KERN_INFO "timer call work\n");
	queue_work( my_wq, work );
	printk(KERN_INFO "modify timer\n");
	if((ll_list_size())>0){
		mod_timer(&intr_timer,jiffies+5*HZ);
	}
	//mod_timer(&intr_timer,jiffies+5*HZ);
}
/* Function to Initialize Timer*/
void initialize_timer(void){
	int wait_time = 5;
	init_timer(&intr_timer);
	intr_timer.expires = jiffies+wait_time*HZ;
	intr_timer.data = intr_timer.expires;
	intr_timer.function = timer_callback;
	//printk(KERN_INFO "adding timer\n");
	add_timer(&intr_timer); 
}

int init_workqueue(void)
{
	//printk(KERN_INFO "workqueue init called\n");
	initialize_timer();//Initializing timer
	create_work_queue(); //calling workqueue
	//printk(KERN_INFO "workqueue init done\n");
	//queue_work( my_wq, &wk1->work );
	return 0;
}

void cleanup_workqueue(void)
{
    //printk(KERN_INFO "cleanup_workqueue called\n");
	del_timer(&intr_timer);
	cancel_work_sync( work );
        flush_workqueue(my_wq);
	destroy_workqueue( my_wq );
}

