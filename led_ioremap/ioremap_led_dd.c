/* LED driver using ioremap function*/
/* ioremap_led_dd.c*/

#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/init.h>
#include <asm/io.h> //outb()
#include <asm/uaccess.h> //copy_from_user()

#define DEV_NAME "/dev/iom_led"
#define IOM_LED_MAJOR_NUM 246
#define IOM_LED_ADDRESS 0xA8000000 //physical address for LED

MODULE_LICENSE("GPL");
MODULE_AUTHOR("HGU");

int iom_led_init(void);
void iom_led_exit(void);
module_init(iom_led_init);
moudel_exit(iom_led_exit);

static int ledport_usage = 0;
static unsigned char *iom_led_addr;

ssize_t iom_led_write(struct file *, const char __user *, size_t, loff_t *);
int iom_led_open(struct inode *, struct file *);
int iom_led_release(struct inode *, struct file *);


struct file_operations  iom_led_fops = {
	.open = iom_led_open,
	.release = iom_led_release,
	.write = iom_led_write,
};

int major_num = 0;

int iom_led_open(struct inode *inode, struct file *filp)
{
	if(ledport_usage != 0){
		return -EBUSY;
	}
	ledport_usage = 1;
	return 0;
}

int iom_led_release(struct inode *inode, struct file *filp)
{
	ledport_usage = 0;
	return 0;
}
ssize_t iom_led_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	unsigned char led_data;
	
	if(copy_from_user(&led_data, buf, count))
		return -EFAULT;
	outb(led_data, (unsigned int) iom_led_addr);

	return count;
}

int __init iom_led_init(void){

	major_num = register_chrdev(IOM_LED_MAJOR_NUM, DEV_NAME, &iom_led_fops);

	if(major_num < 0){
		printk(KERN_WARNING"%sCan't get or assign major number...%d\n", DEV_NAME, IOM_LED_MAJOR_NUM);
		return major_num;
	}

	printk("Success to load the device %s. Major number is %d\n", DEV_NAME, major_num);

	iom_led_addr = ioremap(IOM_LED_ADDRESS, 0x02);

	return 0;
}
void __exit iom_led_exit(void){
	iounmap(iom_led_addr);

	unregister_chrdev(major_num, DEV_NAME);
	printk("Success to unload the device %s...\n", DEV_NAME);
}
