#ifndef CHAR_DEV_H
#define CHAR_DEV_H

// function signatures are incomplete
int cd_initialize(void);
int cd_cleanup(void);
int open(struct inode *, struct file *);
int close(struct inode *, struct file *);
int mmap_callback(struct file *, struct vm_area_struct *);

#endif //CHAR_DEV_Hstruct file *fp, struct vm_area_struct *vma
