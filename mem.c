#include "mem.h"
#include "structure.h"
char *mmap_buf = NULL;
int mem_index = 0;


int mm_initialize(void)
{
	printk(KERN_INFO "Alloating memory for mmap_buffer\n");
	mmap_buf = vmalloc(MMAP_BUF_SIZE);
    // PG_reserved bit is deprecated and hence not needed any more
	if(!mmap_buf)
	{
		printk(KERN_INFO "Failed to allocate mmap buffer\n");
		return FAIL;
	}
	return SUCCESS;
}

int mm_add_data(sampling_data *data)
{
	if(mem_index < MAX_SAMPLES) {
		sampling_data *item = NULL;
		if (!data)
		{
			printk(KERN_INFO "mm_add_data: data is NULL\n");
			return FAIL;
		}
		item 	= (sampling_data *)(mmap_buf + mem_index*sizeof(sampling_data));
		item->jiff_value        = data->jiff_value;
		item->min_flt = data->min_flt;
		item->maj_flt = data->maj_flt;
		item->cpu_util  = data->cpu_util;
		mem_index++; 
		if((mem_index % 10) == 0)
			printk(KERN_INFO "Sample count %d added\n", mem_index);
	}
	return SUCCESS;
}

void *mm_get_buffer(void)
{
	return (void*)mmap_buf;
}

int mm_cleanup(void)
{
	printk(KERN_INFO "FREEING mmap_buffer\n");
	if(mmap_buf)
	{
		vfree(mmap_buf);
	}
	return SUCCESS;
}
