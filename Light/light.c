#include <stdio.h>
#include <fcntl.h>
//#include <linux/i2c-dev.h>
#include <errno.h>
#define I2C_ADDR 0x23
#define I2C_SLAVE   0x0703
int main(void)
{
    int fd;
    char buf[3];
    char val,value;
    float flight;
    fd=open("/dev/i2c-1",O_RDWR);
    if(fd<0)
    {
        printf("fd<0:%s\r\n",strerror(errno)); return 1;
    }
    if(ioctl( fd,I2C_SLAVE,I2C_ADDR)<0 )
    {
        printf("SLAVE<0 : %s\r\n",strerror(errno));return 1;
    }
    val=0x01;
    if(write(fd,&val,1)<0)
    {
        printf("write 0x01\r\n");
    }
    val=0x11;
    if(write(fd,&val,1)<0)
    {
        printf("write 0x11\r\n");
    }
    usleep(200000);
    if(read(fd,&buf,3)){
        flight=(buf[0]*256+buf[1])*0.5/1.2;
        printf("lux: %6.2flx\r\n",flight);
    }
    else{
        printf("??\r\n");
    }
}
