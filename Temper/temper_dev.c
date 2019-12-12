#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/timer.h>

#include <asm/mach/map.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#define TEMPER_MAJOR_NUMBER	504
#define TEMPER_MINOR_NUMBER 100
#define	TEMPER_DEV_NAME	"temper"

#define GPIO_BASE_ADDR	0x3F200000
#define GPFSEL1	0x04
#define	GPSET0	0x1c
#define	GPCLR0	0x28
#define GPLEV0	0x34

#define IOCTL_MAGIC_TEMPER	'w'
#define IOCTL_CMD_TEMPER	_IOR(IOCTL_MAGIC_TEMPER, 0, int)
#define IOCTL_CMD_LOG		_IOW(IOCTL_MAGIC_TEMPER, 1, int)

static void __iomem *gpio_base;
volatile unsigned int *gpsel1;
volatile unsigned int *gpset0;
volatile unsigned int *gpclr0;
volatile unsigned int *gplev0;

#define DATA_NUM 40

int temper_open(struct inode *inode, struct file *filp){
	printk(KERN_ALERT "Temper driver open!!.....temper_open(), ioremap()\n");
	
	gpio_base = ioremap(GPIO_BASE_ADDR, 0x60);
	gpsel1 = (volatile unsigned int *)(gpio_base + GPFSEL1);
	gpset0 = (volatile unsigned int *)(gpio_base + GPSET0);
	gpclr0 = (volatile unsigned int *)(gpio_base + GPCLR0);
	gplev0 = (volatile unsigned int *)(gpio_base + GPLEV0);
	return 0;
}

int temper_release(struct inode *inode, struct file *filp){
	printk(KERN_ALERT "Temper dirver closed!!......temper_release(), iounmap()\n");
	iounmap((void *)gpio_base);
	return 0;
}

long temper_ioctl(struct file *filp, unsigned int cmd, unsigned long arg){
	int i;
	int tmp_i;
	int u_times=0;
	int data[5]={0,0,0,0,0};
	
	switch(cmd){
		// Print Log
		case IOCTL_CMD_LOG:
			if(arg == 1)
				printk(KERN_ALERT "Temper Received\n");
			else if(arg == 0)
				printk(KERN_ALERT "Temper Break\n");
			else
				printk(KERN_ALERT "IOCTL_CMD_LOG error!!\n");
			break;
		
		// Temperature START(gpio 4)
		case IOCTL_CMD_TEMPER:
			printk(KERN_ALERT "START\n");
			for(i=0;i<5;i++) {
				data[i]=0;		//RESET DATA
			}
	
			*gpsel1 |= (1<<24);	//gpio18 output mode
			
			//First signal is Translate start signal(Low 18ms, High 20~40us)
			*gpclr0 |= (1<<18);
			mdelay(18);
			*gpset0 |= (1<<18);
			udelay(30);
			//Response Signal so gpio18 change input mode
			*gpsel1 &= ~(1<<24);
			//ready to output signal
			
			
			while(u_times < 149){
				int received_value = (*gplev0) & (0x01 << 18);
				if(received_value) break;
				udelay(1);
				u_times++;
			}
			//Fail to read
			if(u_times==149){
				printk(KERN_ALERT "FAILED ONE\n");
				return 0;
			}
			
			u_times = 0;
			udelay(100);
			//ready to output signal = 80us, response signal = 50us
			//Read Data 40bit
			for(i=0;i<DATA_NUM;i++){
				u_times = 0;
				
				//ready to output signal
				while(u_times < 149){
					int received_value = (*gplev0) & (0x01 << 18);
					if(received_value) break;
					udelay(1);
					u_times++;
				}
				//Fail to read
				if(u_times==149){
					printk(KERN_ALERT "FAILED TWO\n");
					return 0;
				}
				u_times = 0;
				
				//Read Temperature and Moisture Data
				while((*gplev0) & (0x01 << 18)){		
					udelay(1);
					u_times++;
					
					//Fail to read
					if(u_times == 149){
						printk(KERN_ALERT "FAILED THREE\n");
						return 0;
					}
				}
				tmp_i = i/8;
				data[tmp_i] = data[tmp_i] << 1;
				printk(KERN_ALERT "u_times: %d\n",u_times);
				if(u_times > 25){
					data[tmp_i] = data[tmp_i] + 1;
				}
			}
			//Parity check
			if(data[4]==((data[0] + data[1] + data[2] + data[3]) % 256)){
				//Success
				char buf[30];
				printk(KERN_ALERT "%d %d %d %d\n",data[0],data[1],data[2],data[3]);
				sprintf(buf,"%d %d\n",data[2],data[3]);
				copy_to_user(arg, buf, sizeof(buf));
				return 0;		
			}else{
				//FAILED
				printk(KERN_ALERT "Data Reading Failed\n");
				char failed_value='X';
				copy_to_user(arg,&failed_value,sizeof(char));
				return 0;
			}
			break;
		default:
			printk(KERN_ALERT "ioctl : command error\n");
	}
	return 0;
}

static struct file_operations temper_fops = {
	.owner = THIS_MODULE,
	.open = temper_open,
	.release = temper_release,
	.unlocked_ioctl = temper_ioctl
};

int __init temper_init(void){
	if(register_chrdev(TEMPER_MAJOR_NUMBER, TEMPER_DEV_NAME, &temper_fops) < 0)
		printk(KERN_ALERT "TEMPER driver initialization fail\n");
	else
		printk(KERN_ALERT "TEMPER dirver initialization success.......Temper_init(), register()\n");
	return 0;
}

void __exit temper_exit(void){
	unregister_chrdev(TEMPER_MAJOR_NUMBER, TEMPER_DEV_NAME);
	printk(KERN_ALERT "TEMPER driver exit done......temper_exit(), unregister()\n");
}

module_init(temper_init);
module_exit(temper_exit);
