/*
 This kernel module will be run from a namespace where source pid and target pid namespace both are visible
 and the pids in the sysfs file will be from the common namespace.
        A
       / \
      B   C
Here A, B and C all belong to different PID namespace.
So if we want to change parent of B(source PID) from A to C(target pid), we will specify pid of B and C from the namespace of A.
*/

#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/sched.h>
#include<linux/exit.h>
#include<linux/slab.h>
#include<linux/sched/task.h>
#include<linux/sched/signal.h>
#include<linux/delay.h>
#include<linux/ptrace.h>
#include<linux/uaccess.h>
#include<linux/device.h>

#include "changeParent.h"

struct parent_change_info {
    pid_t source_pid;   
    pid_t target_pid;
    
    struct task_struct* source_task;
    struct task_struct* target_task;
    
    pid_t init_ns_source_pid;
    pid_t init_ns_target_pid;
};

struct parent_change_info* pci;
static int major;
atomic_t  device_opened;
static struct class *migrate_class;
struct device *migrate_device;
static char *d_buf = NULL;

// Kernel global variables
extern pid_t parent_pid_h;      // used just for debugging
extern spinlock_t waitlist_lock;
extern struct task_struct* migrating_task_h;

#define __set_task_state(tsk, state_value)				\
	do {								\
		debug_normal_state_change((state_value));		\
		WRITE_ONCE(tsk->__state, (state_value));		\
	} while (0)

int deschedule_proc(struct task_struct* task) {

    might_sleep();                                                                                                                                                                  

    /*
            Not a good approach, some other process from user space can send a SIGCONT signal and resume the process.
            We want suspension of the process completely hidden from the user space.

            Idea to try: You can remove the task from the run queue, save it somewhere and put it back later.
            (https://stackoverflow.com/questions/13102540/how-to-suspend-a-task-from-within-a-kernel-module-linux)
            In kernel/sched/core.c look for deactivate_task() and activate_task().  --> If function not exported create copies and use
            Maybe setting the state to TASK_STOPPED works as well

    */
    send_sig(SIGSTOP, task, 0);
    printk("Successfully send the signal to the process: %d\n", task->pid);
    //while(task->__state != TASK_STOPPED) {
      //msleep(2); 
    // }
    // task_lock(task);
    // __set_task_state(task, TASK_INTERRUPTIBLE);
    // task_unlock(task);
 
    /*
            deactivate_task is present in kernel/sched/sched.h file which is not in include path :(
    */                                                                                                                                                       
    // deactivate_task(task, this_rq());

    return 0;
}

int reschedule_proc(struct task_struct* task) {

    // Release the lock after work completion and let the process continue
    task_lock(task);
    __set_task_state(task, TASK_RUNNING);
    task_unlock(task);
    send_sig(SIGCONT, task, 0);

    return 0;
}

int detach_process(struct task_struct* task) {
    int err;

    // deschedule the process or wait until the process itself gets descheduled
    err = deschedule_proc(task);

    /*
        Check whether parent is waiting or not already at this moment.
    */

    spin_lock(&waitlist_lock);
    
    // if parent is not waiting
    if(list_empty(&task->parent->signal->wait_chldexit.head)) {
        task_lock(task);
        // need to hold tasklist_lock(export it from kernel module)
        detach_pid(task, PIDTYPE_PID);
        detach_pid(task, PIDTYPE_TGID);
        detach_pid(task, PIDTYPE_PGID);
        detach_pid(task, PIDTYPE_SID);

        list_del_rcu(&task->tasks);
	    list_del_init(&task->sibling);
        task_unlock(task);
        spin_unlock(&waitlist_lock);
    }
    else {
        spin_unlock(&waitlist_lock);
	    task_lock(task);
        task->exit_state = EXIT_MIGRATE;
        send_sig(SIGCHLD, task->parent, 1);
        task_unlock(task);
    }

    return 0;
}

int attach_process(struct task_struct* source, struct task_struct* target) {

        // Attach the process to  the target process as a parent.

        // Make a ptrace call for vfork in the target process

        // restart the process again
        reschedule_proc(source);

        return 0;
}

// Chardev functions
static int device_open(struct inode *inode, struct file *file)
{
        atomic_inc(&device_opened);
        try_module_get(THIS_MODULE);
        printk(KERN_INFO "Device opened successfully\n");
        return 0;
}

static int device_release(struct inode *inode, struct file *file)
{
        atomic_dec(&device_opened);
        module_put(THIS_MODULE);
        printk(KERN_INFO "Device closed successfully\n");

        return 0;
}

static ssize_t device_read(struct file *filp,
                           char *buffer,
                           size_t length,
                           loff_t * offset){
    sprintf(d_buf, "%d %d\n", pci->source_pid, pci->target_pid);
    if(copy_to_user(buffer, d_buf, 4096)){
        printk("Error in writing to userspace\n");
    	return -EINVAL;
    }

    return 4096;
}

static ssize_t
device_write(struct file *filp, const char *buf, size_t len, loff_t * off){
    
    int err;
    
    if(copy_from_user(d_buf, buf, len)){
		printk("Error in reading from userspace\n");
        return -EINVAL;
    }

    err = sscanf(d_buf, "%d %d", &pci->source_pid, &pci->target_pid);

    if (err == -1)
            return -EINVAL;
    
    printk("Source: %d, Target: %d\n", pci->source_pid, pci->target_pid);

    // Now get task struct of source and target pid
    pci->source_task = pid_task(find_vpid(pci->source_pid), PIDTYPE_PID);
    pci->target_task = pid_task(find_vpid(pci->target_pid), PIDTYPE_PID);
    migrating_task_h = pci->source_task;

    if(pci->source_task == NULL || pci->target_task == NULL) {
            printk("Error in getting the task struct from pid of the process\n");
            return -EINVAL;
    }

    pci->init_ns_source_pid = task_pid_nr(pci->source_task); // pid as seen from the global init namespace; it will be used for all future references
    pci->init_ns_target_pid = task_pid_nr(pci->target_task);

    // symbol exported from kernel/exit.c
    parent_pid_h = task_pid_nr(pci->source_task->parent); // pid as seen from the global init namespace
    return len;
}

long device_ioctl(struct file *file,	
		 unsigned int ioctl_num,
		 unsigned long ioctl_param)
{   
    int ret = 0;
    switch (ioctl_num) {
        case IOCTL_DETACH:
            ret = detach_process(pci->source_task);
            printk("Global PID of child process after detach: %d\n", task_pid_nr(pci->source_task));
            break;
        
        case IOCTL_ATTACH:
            ret = attach_process(pci->source_task, pci->target_task);
            break;

        default:
            break;
    }

    return ret;
}

static struct file_operations fops = {
        .read = device_read,
        .write = device_write,
		.unlocked_ioctl = device_ioctl,
        .open = device_open,
        .release = device_release,
};

static char *migrate_devnode(struct device *dev, umode_t *mode)
{
        if (mode && dev->devt == MKDEV(major, 0))
                *mode = 0666;
        return NULL;
}

int init_module(void)
{       
    int err;

    // Register chardev
    major = register_chrdev(0, DEVNAME, &fops);
    err = major;
	if (err < 0) {      
			printk(KERN_ALERT "Registering char device failed with %d\n", major);   
			goto error_regdev;
	}       

    migrate_class = class_create(THIS_MODULE, DEVNAME);
	err = PTR_ERR(migrate_class);
	if (IS_ERR(migrate_class))
			goto error_class;

	migrate_class->devnode = migrate_devnode;

	migrate_device = device_create(migrate_class, NULL,
									MKDEV(major, 0),
									NULL, DEVNAME);
	err = PTR_ERR(migrate_device);
	if (IS_ERR(migrate_device))
			goto error_device;

	printk(KERN_INFO "I was assigned major number %d. To talk to\n", major);                                                              
	atomic_set(&device_opened, 0);     

    pci = (struct parent_change_info*) kzalloc(sizeof(struct parent_change_info), GFP_KERNEL);
    
    if(pci == NULL){
            printk("Error in allocating memory(kzalloc)\n");
            goto err_kzalloc;
    }

    d_buf = (char*) kzalloc(4096, GFP_KERNEL);
    printk("All set!!\n");
	return 0;

err_kzalloc:
	device_destroy(migrate_class, MKDEV(major, 0));
error_device:
    class_destroy(migrate_class);
error_class:
    unregister_chrdev(major, DEVNAME);
error_regdev:
    return  err;
}

void cleanup_module(void)
{      
    kfree(d_buf);
    kfree(pci);
    device_destroy(migrate_class, MKDEV(major, 0));
    class_destroy(migrate_class);
    unregister_chrdev(major, DEVNAME);
    printk("Goodbye Kernel\n");
}

MODULE_LICENSE("GPL");
