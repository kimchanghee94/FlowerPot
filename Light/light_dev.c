#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/delay.h>

#include <asm/mach/map.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#define LIGHT_MAJOR_NUMBER	501
#define LIGHT_MINOR_NUMBER 100
#define	LIGHT_DEV_NAME	"light"

#define GPIO_BASE_ADDR	0x3F200000
#define GPFSEL1	0x04
#define	GPSET0	0x1c
#define	GPCLR0	0x28

#define IOCTL_MAGIC_NUMBER	'j'
#define IOCTL_CMD_LIGHT		_IOR(IOCTL_MAGIC_NUMBER, 0, int)
#define IOCTL_CMD_LOG		_IOW(IOCTL_MAGIC_NUMBER, 1, int)

static void __iomem *gpio_base;
volatile unsigned int *gpsel0;
volatile unsigned int *gpset0;
volatile unsigned int *gpclr0;

int light_open(struct inode *inode, struct file *filp){
	printk(KERN_ALERT "Light driver open!!.....\n");
	
	gpio_base = ioremap(GPIO_BASE_ADDR, 0x60);
	gpsel0 = (volatile unsigned int *)(gpio_base + GPFSEL1);
	gpset0 = (volatile unsigned int *)(gpio_base + GPSET0);
	gpclr0 = (volatile unsigned int *)(gpio_base + GPCLR0);
	
	*gpsel0 |= (1<<21);	// gpio 12 is setted output mode
	return 0;
}

int light_release(struct inode *inode, struct file *filp){
	printk(KERN_ALERT "Light dirver closed!!......\n");
	iounmap((void *)gpio_base);
	return 0;
}

// Data Send
long light_ioctl(struct file *filp, unsigned int cmd, unsigned long arg){
	int i=0;
	switch(cmd){
		// Print Log
		case IOCTL_CMD_LOG:
			if(arg == 1)
				printk(KERN_ALERT "DATA Received\n");
			else if(arg == 0)
				printk(KERN_ALERT "DATA Break\n");
			else
				printk(KERN_ALERT "IOCTL_CMD_LOG error!!\n");
			break;
		
		// Light START(gpio 4)
		case IOCTL_CMD_LIGHT:
			
			break;		
		default:
			printk(KERN_ALERT "ioctl : command error\n");
	}
	return 0;
}

static struct file_operations light_fops = {
	.owner = THIS_MODULE,
	.open = light_open,
	.release = light_release,
	.unlocked_ioctl = light_ioctl
};

int __init light_init(void){
	if(register_chrdev(LIGHT_MAJOR_NUMBER, LIGHT_DEV_NAME, &light_fops) < 0)
		printk(KERN_ALERT "LIGHT driver initialization fail\n");
	else
		printk(KERN_ALERT "LIGHT dirver initialization success.......\n");
	return 0;
}

void __exit light_exit(void){
	unregister_chrdev(LIGHT_MAJOR_NUMBER, LIGHT_DEV_NAME);
	printk(KERN_ALERT "LIGHT driver exit done......\n");
}

module_init(light_init);
module_exit(light_exit);
