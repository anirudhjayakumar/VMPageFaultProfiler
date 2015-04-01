#include <linux/slab.h> // for kmalloc
#include <linux/workqueue.h> // workqueue_struct
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/mutex.h>
#include <linux/pid.h>
#include "structure.h"
#include "linklist.h"
#include <linux/sched.h>
#include <linux/fs_struct.h>
#include "mem.h"
#include "mp3_given.h"
#include "workqueue.h"

static struct timer_list intr_timer;
/*Work Queue:- my_wq*/
static struct workqueue_struct *my_wq = NULL;
//creating work element:- work
struct work_struct *work = NULL;


void wq_modify_timer(void)
{
	mod_timer(&intr_timer, jiffies + msecs_to_jiffies(50)); // 20 samples per second = 50 ms	
}


//Work handler 
void work_handler( struct work_struct *work )
{
	int *pids = NULL;
    int count = 0;
   	int index,pid;
    ulong curr_jiff,prev_jiff;
    ulong min_flt,maj_flt,utime,stime;
	// initialize sample data structure
    sampling_data sample;
	sample.jiff_value = 0;
	sample.min_flt = 0;
	sample.maj_flt = 0;
	sample. cpu_util = 0;
	
	//get list of pids that are registerd
	ll_get_pids(&pids,&count);
	
    for (index = 0; index <count; ++index)
	{

		pid = pids[index];
        ll_get_last_sample_jiff(pid,&prev_jiff);
		// get statistics of each process
		if( get_cpu_use(pid,&min_flt,&maj_flt,&utime,&stime) == 0 )
		{
			// add up the numbers
			curr_jiff = jiffies;
			sample.min_flt += min_flt;
			sample.maj_flt += maj_flt;
			// floating point is costly. Utilization will be out of 1000
			sample.cpu_util += (utime+stime)*1000/(curr_jiff-prev_jiff);
			// update individual process information
	        ll_update_item(pid,min_flt,maj_flt,utime+stime,curr_jiff); 
		}
		else
		{ 
			//printk(KERN_INFO "get_cpu_use() failed");
			ll_delete_item(pid);
		}
	}

    // aggregrate statistics of all the process
    sample.jiff_value = jiffies; // timestamp jiffy
	// add to buffer
    mm_add_data(&sample);	
	kfree(pids);
}

/** function to create workqueue*/
int wq_create_workqueue(void)
{
	//printk(KERN_INFO "workqueue creation called\n");
	if( !my_wq )
	{
		my_wq= create_workqueue( "mp3_workqueue\n" );
	}
	if ( !my_wq ) {
		printk( KERN_INFO "Error!Workqueue could not be created\n" );
		return FAIL ;
	}
	if ( !work )
	{
		work = (struct work_struct*)kmalloc(sizeof(struct work_struct), GFP_KERNEL );
	}
	if ( work ) 
	{	
		INIT_WORK( work, work_handler );
	} else
	{
        printk( KERN_INFO "Error! Work could not be created\n" );
		return FAIL ;
	}
	return SUCCESS;
}

void wq_destroy_workqueue(void)
{
	cancel_work_sync(work);
	kfree(work);
	work = NULL;
	flush_workqueue(my_wq);
	destroy_workqueue(my_wq);
	my_wq = NULL;
}

void timer_callback(unsigned long data){
	//printk(KERN_INFO "timer call work\n");
	if(my_wq && work)
		queue_work( my_wq, work );
	//printk(KERN_INFO "modify timer\n");
	if((ll_list_size())>0) {
		wq_modify_timer();
	}
}

/* Function to Initialize Timer*/
void initialize_timer(void){
	setup_timer( &intr_timer, timer_callback, 0 );
}

int wq_init_workqueue(void)
{
	printk(KERN_INFO "workqueue init called\n");
	initialize_timer();//Initializing timer
	wq_create_workqueue(); //calling workqueue	
	return 0;
}

void wq_cleanup_workqueue(void)
{
    printk(KERN_INFO "cleanup_workqueue called\n");
	del_timer(&intr_timer);
	wq_destroy_workqueue();
}

