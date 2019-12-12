#include <linux/init.h> 
#include <linux/kernel.h> 
#include <linux/module.h> 
#include <linux/fs.h> 
#include <linux/uaccess.h> 
#include <linux/slab.h> 

#include <asm/mach/map.h> 
#include <asm/uaccess.h> 

#define SOILMOIS_MAJOR_NUMBER   505
#define SOILMOIS_DEV_NAME      "soilmois"

#define GPIO_BASE_ADDRESS   0x3F200000

#define GPFSEL0   0x00
#define GPLEV0   0x34
#define GPSET0   0x1C
#define GPCLR0   0x28

#define SPI_CHANNEL   0   // CH0 in ADC
#define SPI_SPEED   1000000   // 1MHz

#define IOCTL_MAGIC_SOILMOIS   'e'
#define IOCTL_CMD_SET_SPI_ACTIVE      _IOWR(IOCTL_MAGIC_SOILMOIS, 0, int)
#define IOCTL_CMD_SET_SPI_INACTIVE      _IOWR(IOCTL_MAGIC_SOILMOIS, 1, int)

static void __iomem * gpio_base;
volatile unsigned int * gpsel0;
volatile unsigned int * gplev0;
volatile unsigned int * gpset0;
volatile unsigned int * gpclr0;

int soilmois_open(struct inode * inode, struct file * filp){
   printk(KERN_ALERT "soilmois driver open\n");
   
   gpio_base = ioremap(GPIO_BASE_ADDRESS, 0xFF);
   gpsel0 = (volatile unsigned int *)(gpio_base + GPFSEL0);
   gplev0 = (volatile unsigned int *)(gpio_base + GPLEV0);
   gpset0 = (volatile unsigned int *)(gpio_base + GPSET0);
   gpclr0 = (volatile unsigned int *)(gpio_base + GPCLR0);
   
   return 0;
}

int soilmois_release(struct inode * inode, struct file * filp){
   printk(KERN_ALERT "soilmois driver close\n");
   iounmap((void *)gpio_base);
   return 0;
}

// arg is adcChannel
long soilmois_ioctl(struct file * filp, unsigned int cmd, unsigned long arg){
   unsigned char buff[3];   // communication with ADC
   int get_value;
   
   switch (cmd){
      case IOCTL_CMD_SET_SPI_ACTIVE:
         // set RBP gpio to output mode
         *gpsel0 |= (1 << 24); // BCM gpio 8 is setted output mode
         
         // LOW : chip select active in adc
         *gpclr0 |= (1 << 8);
         break;
      case IOCTL_CMD_SET_SPI_INACTIVE:
         // High : chip select inactive in adc
         *gpset0 |= (1 << 8);
         break;
      default:
         printk(KERN_ALERT "ioctl : command error\n");
   }
   return 0;
}

static struct file_operations soilmois_fops = {
   .owner = THIS_MODULE,
   .open = soilmois_open,
   .release = soilmois_release,
   .unlocked_ioctl = soilmois_ioctl
};

int __init soilmois_init (void){
   if(register_chrdev(SOILMOIS_MAJOR_NUMBER, SOILMOIS_DEV_NAME, &soilmois_fops) < 0)
      printk(KERN_ALERT "soilmois driver initialization failed\n");
   else
      printk(KERN_ALERT "soilmois driver suceed\n");
   
   return 0;
}

void __exit soilmois_exit(void){
   unregister_chrdev(SOILMOIS_MAJOR_NUMBER, SOILMOIS_DEV_NAME);
   printk(KERN_ALERT "soil mois driver exit");
}

module_init(soilmois_init);
module_exit(soilmois_exit);
