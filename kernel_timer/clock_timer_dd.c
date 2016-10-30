/* LED: Kernel Timer Example
   FILE: led_timer_driver.c
*/
#ifndef __LED_KENEL_TIMER_DRIVER_
#define __LED_KENEL_TIMER_DRIVER_

#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/init.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/slab.h>
#include <linux/timer.h>
typedef struct {
	struct timer_list timer;
}__attribute((packed))KERNEL_TIMER_STRUCT;
#endif

#define DEV_NAME "/dev/clock_timer"
#define CLOCK_TIMER_MAJOR_NUM 247
#define TIME_STEP (10*HZ/10) /* 1.0 sec */

static KERNEL_TIMER_STRUCT *ptimermgr = NULL;

static unsigned char tick = 1;

MODULE_LICENSE("GPL");
MODULE_AUTHOR("AIN");

int clock_timer_init(void);
void clock_timer_exit(void);
module_init(clock_timer_init);
module_exit(clock_timer_exit);

void clock_timer_timeover(unsigned long arg); /* time function */
void clock_timer_register(KERNEL_TIMER_STRUCT *pdata, unsigned long timeover);

ssize_t clock_timer_write (struct file *flip, const char *buf, size_t count, loff_t *f_pos);
ssize_t clock_timer_read (struct file *flip, const char *buf, size_t count, loff_t *f_pos);
int clock_timer_open (struct inode *, struct file *);
int clock_timer_release (struct inode *, struct file *);

struct file_operations clock_timer_fops = {
	.open = clock_timer_open,
	.release = clock_timer_release,
	.write = clock_timer_write,
	.read = clock_timer_read,
};

int clock_timer_open (struct inode *inode, struct file *flip)
{
	
	return 0;
}


int clock_timer_release (struct inode *inode, struct file *filp)
{

	return 0;
}
ssize_t clock_timer_write (struct file *flip, const char *buf, size_t count, loff_t *f_pos)
{
	
	clock_timer_register(ptimermgr, TIME_STEP);
	
	return count;
}

ssize_t clock_timer_read (struct file *flip, const char *buf, size_t count, loff_t *f_pos)
{
	if (copy_to_user(buf, &tick, count))
		return -EFAULT;
	return count;
}

int __init clock_timer_init(void)
{
	int major_num;

	major_num = register_chrdev(CLOCK_TIMER_MAJOR_NUM, DEV_NAME, &clock_timer_fops);

	printk("Success to load the device %s, Major number is %d\n", DEV_NAME, CLOCK_TIMER_MAJOR_NUM);
	
	ptimermgr = kmalloc(sizeof(KERNEL_TIMER_STRUCT), GFP_KERNEL);
	if(ptimermgr == NULL)
		return -ENOMEM;
	
	memset(ptimermgr, 0, sizeof(KERNEL_TIMER_STRUCT));
	
	//led_timer_register(ptimermgr, TIME_STEP);
		
	return 0;
}

void clock_timer_register(KERNEL_TIMER_STRUCT *pdata, unsigned long timeover)
{
	//printk("timer_register:%d\n", tick);
	init_timer(&(pdata->timer));
	pdata->timer.expires = get_jiffies_64() + timeover;
	pdata->timer.function = clock_timer_timeover; /* handler of the timeout */
	pdata->timer.data = (unsigned long) pdata; /* argument of the handler */
	add_timer(&(pdata->timer));
}

void clock_timer_timeover(unsigned long arg)
{
	KERNEL_TIMER_STRUCT *pdata = NULL;

	//printk("timer_timeover:%d\n", tick);	
	pdata = (KERNEL_TIMER_STRUCT *) arg;

	tick = ~tick;
	clock_timer_register(pdata, TIME_STEP);
}

void __exit clock_timer_exit(void)
{
	
	if(ptimermgr != NULL) {
		del_timer(&(ptimermgr->timer));
		kfree(ptimermgr);
	}

	unregister_chrdev(CLOCK_TIMER_MAJOR_NUM, DEV_NAME);
	printk("Success to unload the device %s..\n", DEV_NAME);
}

