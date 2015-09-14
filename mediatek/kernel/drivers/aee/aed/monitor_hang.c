/* 
 * (C) Copyright 2010
 * MediaTek <www.MediaTek.com>
 *
 * Android Exception Device
 *
 */
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/hardirq.h>
#include <linux/init.h>
#include <linux/kallsyms.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/poll.h>
#include <linux/proc_fs.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/vmalloc.h>
#include <linux/disp_assert_layer.h>
#include <mach/system.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>
#include <linux/workqueue.h>
#include <linux/kthread.h>
#include <linux/aee.h>
#include <linux/seq_file.h>
#include "aed.h"
#include "../ipanic/ipanic.h"
#include <mach/mt_boot.h>

/******************************************************************************
 * FUNCTION PROTOTYPES
 *****************************************************************************/
static long monitor_hang_ioctl(struct file *file, unsigned int cmd, unsigned long arg);



/******************************************************************************
 * hang detect File operations
 *****************************************************************************/
static int monitor_hang_open(struct inode *inode, struct file *filp)
{
	//xlog_printk(ANDROID_LOG_DEBUG, AEK_LOG_TAG, "%s\n", __func__);	
	return 0;
}

static int monitor_hang_release(struct inode *inode, struct file *filp)
{
	//xlog_printk(ANDROID_LOG_DEBUG, AEK_LOG_TAG, "%s\n", __func__);	
	return 0;
}

static unsigned int monitor_hang_poll(struct file *file, struct poll_table_struct *ptable)
{
	xlog_printk(ANDROID_LOG_DEBUG, AEK_LOG_TAG, "%s\n", __func__);
	return 0;
}

static ssize_t monitor_hang_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	xlog_printk(ANDROID_LOG_DEBUG, AEK_LOG_TAG, "%s\n", __func__);
	return 0;
}

static ssize_t monitor_hang_write(struct file *filp, const char __user *buf, size_t count,
			    loff_t *f_pos)
{
	
	xlog_printk(ANDROID_LOG_DEBUG, AEK_LOG_TAG, "%s\n", __func__);
	return 0;
}


// QHQ RT Monitor    
extern void aee_kernel_RT_Monitor_api(int lParam);
#define AEEIOCTL_RT_MON_Kick _IOR('p', 0x0A, int)
// QHQ RT Monitor    end




/*
 * aed process daemon and other command line may access me 
 * concurrently
 */
static long monitor_hang_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	if(cmd == AEEIOCTL_WDT_KICK_POWERKEY)
	{
		return ret;
	}	
	
// QHQ RT Monitor    
	if(cmd == AEEIOCTL_RT_MON_Kick)
	{
		xlog_printk(ANDROID_LOG_DEBUG, AEK_LOG_TAG, "AEEIOCTL_RT_MON_Kick ( %d)\n", (int)arg);
		aee_kernel_RT_Monitor_api((int)arg);
		return ret;
	}
// QHQ RT Monitor end
	return ret;
}




// QHQ RT Monitor    
static struct file_operations aed_wdt_RT_Monitor_fops = {
	.owner   = THIS_MODULE,
	.open    = monitor_hang_open,
	.release = monitor_hang_release,
	.poll    = monitor_hang_poll,
	.read    = monitor_hang_read,
	.write   = monitor_hang_write,
	.unlocked_ioctl   = monitor_hang_ioctl,
};

static struct miscdevice aed_wdt_RT_Monitor_dev = {
	.minor   = MISC_DYNAMIC_MINOR,
	.name    = "RT_Monitor",
	.fops    = &aed_wdt_RT_Monitor_fops,
};



// bleow code is added for monitor_hang_init
static int	monitor_hang_init (void) ;

static int	hang_detect_init (void) ;
// bleow code is added for hang detect



static int __init monitor_hang_init(void)
{
	int err = 0;
	
// bleow code is added by QHQ  for hang detect	
	err = misc_register(&aed_wdt_RT_Monitor_dev);
	if(unlikely(err)) {
		xlog_printk(ANDROID_LOG_ERROR, AEK_LOG_TAG, "aee: failed to register aed_wdt_RT_Monitor_dev device!\n");
		return err;
	}

	hang_detect_init () ;
// bleow code is added by QHQ  for hang detect
// end

	return err;
}

static void __exit monitor_hang_exit(void)
{
	int err;
	err = misc_deregister(&aed_wdt_RT_Monitor_dev);
	if (unlikely(err))
		xlog_printk(ANDROID_LOG_ERROR, AEK_LOG_TAG, "xLog: failed to unregister RT_Monitor device!\n");
}




// bleow code is added by QHQ  for hang detect
// For the condition, where kernel is still alive, but system server is not scheduled.

#define HD_PROC "hang_detect"

//static DEFINE_SPINLOCK(hd_locked_up);
#define HD_INTER 30

static int hd_detect_enabled = 0 ;
static int hd_timeout = 0x7fffffff ;
static int hang_detect_counter = 0x7fffffff ;


static int FindTaskByName (char *name) {
	struct task_struct * task ;
	for_each_process(task) {
		if (task && (strcmp(task->comm, name)==0)) {
			printk("[Hang_Detect] %s found:%d.\n", name, task->pid);
			return task->pid;
		}
	}
	printk("[Hang_Detect] system_server not found!\n");
	return -1 ;
}
extern void show_stack(struct task_struct *tsk, unsigned long *sp);
static void ShowStatus (void) {
	struct task_struct * task ;
	for_each_process(task) {
		printk_deferred("[Hang_Detect] %s found:%d.,RT[%lld]\n", task->comm, task->pid, sched_clock());
		show_stack(task,NULL) ;
	}
}

static int hang_detect_thread(void *arg) {

	//unsigned long flags;
	struct sched_param param = { .sched_priority = RTPM_PRIO_WDT};

	printk("[Hang_Detect] hang_detect thread starts.\n");

	sched_setscheduler(current, SCHED_FIFO, &param);

	while (1)
	{
		if ((1==hd_detect_enabled) && (FindTaskByName("system_server")!=-1)) {
			printk("[Hang_Detect] hang_detect thread counts down %d:%d.\n", hang_detect_counter, hd_timeout);


			if (hang_detect_counter<=0)	{
				ShowStatus () ;
			}
			if (hang_detect_counter==0)	{
				printk ("[Hang_Detect] we should triger	HWT	...	\n") ;

#ifdef CONFIG_MT_ENG_BUILD
				aee_kernel_warning("\nCR_DISPATCH_KEY:SS Hang\n", "we triger	HWT");
				msleep (10*1000) ;
#else
				aee_kernel_warning("\nCR_DISPATCH_KEY:SS Hang\n", "we triger	HWT");
				msleep (10*1000) ;
				local_irq_disable () ;
			
				while (1) 
				;

				BUG	() ;
#endif
			}

			hang_detect_counter -- ;
		}
		else {
			// incase of system_server restart, we give 2 mins more.(4*HD_INTER)
			if (1==hd_detect_enabled) {
				hang_detect_counter = hd_timeout + 4 ;
				hd_detect_enabled = 0 ;
			}
			printk("[Hang_Detect] hang_detect disabled.\n") ;
		}
		
		msleep((HD_INTER) * 1000);
	}
	return 0 ;
}

void hd_test (void)
{
	hang_detect_counter = 0 ;
	hd_timeout = 0 ;
}

void aee_kernel_RT_Monitor_api(int lParam)
{
	if (0 == lParam) {
		hd_detect_enabled =	0 ;
		hang_detect_counter = hd_timeout ;    
		printk("[Hang_Detect] hang_detect disabled\n") ;
	}
	else if (lParam >0) {
		hd_detect_enabled =	1 ;
		hang_detect_counter	= hd_timeout = ((long)lParam + HD_INTER	-1)	/ (HD_INTER) ;
		printk("[Hang_Detect] hang_detect enabled %d\n", hd_timeout) ;
	}
}

#if 0
static int hd_proc_cmd_read(char *buf, char **start, off_t offset, int count, int *eof, void *data) {

	// with a read of the /proc/hang_detect, we reset the counter.
	int len = 0 ;
	printk("[Hang_Detect] read proc	%d\n", count);

	len = sprintf (buf, "%d:%d\n", hang_detect_counter,hd_timeout) ;

	hang_detect_counter = hd_timeout ;

	return len ;
}

static int hd_proc_cmd_write(struct file *file, const char *buf, unsigned long count, void *data) {

	// with a write function , we set the time out, in seconds.
	// with a '0' argument, we set it to max int
	// with negative number, we will triger a timeout, or for extention functions (future use)

	int counter = 0 ;
	int retval = 0 ;

	retval = sscanf (buf, "%d", &counter) ;

	printk("[Hang_Detect] write	proc %d, original %d: %d\n", counter, hang_detect_counter, hd_timeout);

	if (counter > 0) {
		if (counter%HD_INTER != 0)
			hd_timeout = 1 ;
		else
			hd_timeout = 0 ;

		counter = counter / HD_INTER ;
		hd_timeout += counter ;
	}
	else if (counter == 0)
		hd_timeout = 0x7fffffff ;
	else if (counter == -1)
		hd_test () ;
	else 
		return count ;

	hang_detect_counter = hd_timeout ;

	return count ;
}
#endif

int hang_detect_init(void) {

	struct task_struct *hd_thread ;
	//struct proc_dir_entry *de = create_proc_entry(HD_PROC, 0664, 0);
	
	unsigned char *name = "hang_detect" ;
	
	printk("[Hang_Detect] Initialize proc\n");
	
	//de->read_proc = hd_proc_cmd_read;
	//de->write_proc = hd_proc_cmd_write;

	printk("[Hang_Detect] create hang_detect thread\n");

	hd_thread = kthread_create(hang_detect_thread, NULL, name);
	wake_up_process(hd_thread);    

	return 0 ;
}
// added by QHQ  for hang detect
// end


int aee_kernel_Powerkey_is_press(void)
{
	int ret=0;
	return ret;
}
EXPORT_SYMBOL(aee_kernel_Powerkey_is_press);

/*void aee_kernel_wdt_kick_Powkey_api(const char *module,  int msg)
{
}
EXPORT_SYMBOL(aee_kernel_wdt_kick_Powkey_api);*/


void aee_powerkey_notify_press(unsigned long pressed)
{
}
EXPORT_SYMBOL(aee_powerkey_notify_press);


int aee_kernel_wdt_kick_api(int kinterval)
{
	int ret=0;		
	return ret;
}
EXPORT_SYMBOL(aee_kernel_wdt_kick_api);


module_init(monitor_hang_init);
module_exit(monitor_hang_exit);


MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MediaTek AED Driver");
MODULE_AUTHOR("MediaTek Inc.");
