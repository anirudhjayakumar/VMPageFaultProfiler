#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/vmalloc.h>
#include <linux/mm.h>
#include "mem.h"

static dev_t dev_num;	//variable for device number
static struct class *c; //variable for the device class
static struct cdev c_dev; //variable for the character device structure

static int open(struct inode *i, struct file *f)
{
	return SUCCESS;
}

static int close(struct inode *i, struct file *f)
{
	return SUCCESS;
}

static int mmap_callback(struct file *fp, struct vm_area_struct *vma)
{
        int ret;
        long length = vma->vm_end - vma->vm_start;
        unsigned long start = vma->vm_start;
        char *vmalloc_area_ptr = (char *)mm_get_buffer();
        unsigned long pfn;

        if (length > 128 * PAGE_SIZE)
                return FAIL;

        while (length > 0) {
                pfn = vmalloc_to_pfn(vmalloc_area_ptr);
                if ((ret = remap_pfn_range(vma, start, pfn, PAGE_SIZE,
                                           PAGE_SHARED)) < 0) {
                        return FAIL;
                }
                start += PAGE_SIZE;
                vmalloc_area_ptr += PAGE_SIZE;
                length -= PAGE_SIZE;
        }
        
	return SUCCESS;
}

//structure for file operations
static struct file_operations fops =
{
	.open = open,
	.release = close,
	.mmap = mmap_callback
};


int cd_initialize(void)
{
	//allocating device number to the device driver "mp3"
	int ret = alloc_chrdev_region(&dev_num,0,1,"mp3");
	if(ret != 0){
		printk(KERN_INFO "Error creating character device file");
		return FAIL;
	}
	printk(KERN_INFO "character device created\n");	

	//creating device class
	if((c = class_create(THIS_MODULE, "mp3_class")) == NULL){
		printk(KERN_INFO "Error creating character device class");
		return FAIL;
	}

	//creating device file "node" for character device
	if (device_create(c, NULL, dev_num, NULL, "node") == NULL){
		printk(KERN_INFO "Error creating character device class");
		return FAIL;	
	}
	
	//initializing the character device structure
	cdev_init(&c_dev, &fops);
	
	//adding character device to the system
	if (cdev_add(&c_dev, dev_num, 1) == -1){
		printk(KERN_INFO "Error adding character device");
		return FAIL;
	}

	printk (KERN_INFO "Added Char Dev to system\n");
	return SUCCESS;
}

int cd_cleanup(void)
{
	cdev_del(&c_dev);
	device_destroy(c, dev_num);
	class_destroy(c);
	unregister_chrdev_region(dev_num,1);
	return SUCCESS;
}
