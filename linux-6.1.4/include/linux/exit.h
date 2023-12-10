#ifndef _EXIT_H
#define _EXIT_H
#include <linux/sched.h>
#define debug_h(pid,x, ...) if(pid==parent_pid_h)printk(KERN_CRIT "CRIT: "x" \n", ##__VA_ARGS__)
// #define debug_h(pid,x)

extern pid_t parent_pid_h;
extern spinlock_t waitlist_lock;
extern void exit_notify(struct task_struct *tsk, int group_dead);

#endif
