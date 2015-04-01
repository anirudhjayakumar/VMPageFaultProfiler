#ifndef __MP3_GIVEN_INCLUDE__
#define __MP3_GIVEN_INCLUDE__


struct task_struct* find_task_by_pid(unsigned int nr);

// THIS FUNCTION RETURNS 0 IF THE PID IS VALID. IT ALSO RETURNS THE
// PROCESS CPU TIME IN JIFFIES AND MAJOR AND MINOR PAGE FAULT COUNTS
// SINCE THE LAST INVOCATION OF THE FUNCTION FOR THE SPECIFIED PID.
// OTHERWISE IT RETURNS -1
int get_cpu_use(int pid, unsigned long *min_flt, unsigned long *maj_flt,
         unsigned long *utime, unsigned long *stime);

#endif
