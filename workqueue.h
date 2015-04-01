#ifndef WORKQ_H
#define WORKQ_H

// to be used in module init destroy
int wq_init_workqueue(void);
void wq_cleanup_workqueue(void);


void wq_modify_timer(void);
int  wq_create_workqueue(void);
void wq_destroy_workqueue(void);

#endif
