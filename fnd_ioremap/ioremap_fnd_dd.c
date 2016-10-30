/*Device drive for FND(7-segment display)*/
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/init.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#define IOM_FND_MAJOR_NUM 241
#define DEV_NAME "/dev/iom_fnd"
#define IOM_FND_ADDRESS 0xA8000200

MODULE_LICENSE("GPL");
MODULE_AUTHOR("HGU");

static int fndport_usage = 0;
static unsigned char *iom_fnd_addr;

int init_iom_fnd(void);
void cleanup_iom_fnd(void);
module_init(init_iom_fnd);
moudel_exit(cleanup_iom_fnd);


ssize_t iom_fnd_write(struct file *, const char __user *, size_t, loff_t *);
int iom_fnd_open(struct inode *, struct file *);
int iom_fnd_release(struct inode *, struct file *);


struct file_operations  iom_fnd_fops = {
	.open = iom_fnd_open,
	.release = iom_fnd_release,
	.write = iom_fnd_write,
};

int major_num = 0;

int iom_fnd_open(struct inode *inode, struct file *filp)
{
	if(fndport_usage !=0)	
		return -EBUSY;
	fndport_usage = 1;
	return 0;
}

int iom_fnd_release(struct inode *inode, struct file *filp)
{
	fndport_usage = 0;
	return 0;
}
ssize_t iom_fnd_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	unsigned char fnd_data;

	if(copy_from_user(&fnd_data, buf, count))
		return -EFAULT;
	outb(fnd_data, (unsigned int)iom_fnd_addr);

	return count;
}

int __init init_iom_fnd(void){

	major_num = register_chrdev(IOM_FND_MAJOR_NUM, DEV_NAME, &iom_fnd_fops);

	if(major_num < 0){
		printk(KERN_WARNING"%s: Can't get or assign major number%d\n", DEV_NAME, IOM_FND_MAJOR_NUM);
		return major_num;
	}
	iom_fnd_addr = ioremap(IOM_FND_ADDRESS, 0x1);
	printk("Success to load the device %s. Major number is %d\n", DEV_NAME, IOM_FND_MAJOR_NUM);
	return 0;
}
void __exit cleanup_iom_fnd(void){
	outb(0xFF, (unsigned int)iom_fnd_addr);	/*turn off 7-segment*/

	iounmap(iom_fnd_addr);
	unregister_chrdev(IOM_FND_MAJOR_NUM, DEV_NAME);
	printk("Success to unload the device %s...\n", DEV_NAME);
}
