#define _CRT_SECURE_NO_WARNINGS
#define _REENTRANT
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <math.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/sysmacros.h>

#include <linux/spi/spidev.h>

#define I2C_ADDR 0x23
#define I2C_SLAVE   0x0703
#define POWER_ON	0x01
#define CONTINUOS_HIGH_RES_MODE1 0x10
#define CONTINUOS_HIGH_RES_MODE2 0x11

#define TEMPER_MAJOR_NUMBER	504
#define TEMPER_MINOR_NUMBER	100
#define	TEMPER_DEV_NAME	"temper"
#define TEMPER_DEV_PATH	"/dev/temper"

#define IOCTL_MAGIC_TEMPER		'w'
#define IOCTL_CMD_TEMPER		_IOR(IOCTL_MAGIC_TEMPER, 0, int)

#define SOILMOIS_MAJOR_NUMBER	505
#define SOILMOIS_MINOR_NUMBER	100
#define	SOILMOIS_DEV_NAME		"soilmois"
#define SOILMOIS_DEV_PATH		"/dev/soilmois"

#define IOCTL_MAGIC_SOILMOIS	'e'
#define IOCTL_CMD_SET_SPI_ACTIVE      _IOWR(IOCTL_MAGIC_SOILMOIS, 0, int)
#define IOCTL_CMD_SET_SPI_INACTIVE      _IOWR(IOCTL_MAGIC_SOILMOIS, 1, int)

#define BUF_SIZE 100

static const char *spiDev0 = "/dev/spidev0.0";
static const char *spiDev1 = "/dev/spidev0.1";
static const uint8_t spiBPW = 8;
static const uint16_t spiDelay = 0;

void * temper_msg(void * arg);
void * soilmois_msg(void * arg);
void * light_msg(void * arg);
void error_handling(char * msg);

int main(int argc, char * argv[]){
	int sock;
	struct sockaddr_in serv_addr;
	sock=socket(PF_INET, SOCK_STREAM, 0);
	pthread_t soilmois_thread, temper_thread, light_thread;
	void * thread_return;
	
	if(argc!=3){
		printf("Usage : %s <IP> <port>\n",argv[0]);
		exit(1);
	}
	
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family=AF_INET;
	serv_addr.sin_addr.s_addr=inet_addr(argv[1]);
	serv_addr.sin_port=htons(atoi(argv[2]));
	
	if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))==-1)
		error_handling("connect() error");
			
	pthread_create(&temper_thread, NULL, temper_msg, (void*)&sock);
	pthread_create(&soilmois_thread, NULL, soilmois_msg, (void*)&sock);
	pthread_create(&light_thread, NULL, light_msg, (void*)&sock);
	pthread_join(temper_thread, &thread_return);
	pthread_join(soilmois_thread, &thread_return);
	pthread_join(light_thread, &thread_return);
	close(sock);
	return 0;
}

void *temper_msg(void * arg){  
	int temper_dev;
	int sock = *((int*)arg);
	char name_msg[BUF_SIZE];
	
	temper_dev = makedev(TEMPER_MAJOR_NUMBER, TEMPER_MINOR_NUMBER);
	mknod(TEMPER_DEV_PATH, S_IFCHR | 0666, temper_dev);
	
	temper_dev = open(TEMPER_DEV_PATH, O_WRONLY);
	
	if(temper_dev < 0){
		printf("fail to open temper device driver\n");
		return NULL;
	}
	
	char data[30];
	double high,low;
	while(1){
		high=0,low=0;
		memset(data,0,sizeof(data));
		ioctl(temper_dev, IOCTL_CMD_TEMPER, data);
		high = (double)(data[0]-'0')*10;
		high += (double)(data[1]-'0');
		low = (double)(data[3]-'0');
		high += (0.1*low);
		if(high>100.0 || high<20.0) continue;
		printf("temperature: %.1lf\n",high);
		if(high>29.0){
			write(sock,"1\n",2);
		}else{
			write(sock,"2\n",2);
		}
		sleep(3);
	}
	close(temper_dev);
	return NULL;
}

void *soilmois_msg(void * arg){  
	int mode = 0 & 3;
	int channel = 0 & 1;
	
	int soilmois_dev, spi_dev;
	int sock = *((int*)arg);
	
	soilmois_dev = makedev(SOILMOIS_MAJOR_NUMBER, SOILMOIS_MINOR_NUMBER);
	mknod(SOILMOIS_DEV_PATH, S_IFCHR | 0666, soilmois_dev);
	
	soilmois_dev = open(SOILMOIS_DEV_PATH, O_RDWR);
	spi_dev = open(channel == 0 ? spiDev0 : spiDev1, O_RDWR);
	
	if(soilmois_dev < 0){
		printf("fail to open soilmois device driver\n");
		return NULL;
	}
	if(spi_dev<0){
		printf("fail to open spidev0.0\n");
		return (void*)-1;
	}
	
	// Implementation
	int received_value = 0;
	unsigned char buff[3];	// communication with ADC
	struct spi_ioc_transfer spi;	// in "spidev.h"
	
	while(1){
		ioctl(soilmois_dev, IOCTL_CMD_SET_SPI_ACTIVE, &channel);
		
		// SYNC msg
		buff[0] = 0x06 | ((channel & 0x07) >> 7);
		buff[1] = ((channel & 0x07) << 6);
		buff[2] = 0x00;
		
		memset(&spi, 0, sizeof(spi));
		
		spi.tx_buf = (unsigned long)buff;
		spi.rx_buf = (unsigned long)buff;
		spi.len = 3;
		spi.delay_usecs = spiDelay;
		spi.speed_hz = 1000000;	// 1MHz
		spi.bits_per_word = spiBPW;
		
		ioctl(spi_dev, SPI_IOC_MESSAGE(1), &spi);
		buff[1] = 0x0F & buff[1];
		received_value = (buff[1] << 8) | buff[2];
		
		ioctl(soilmois_dev, IOCTL_CMD_SET_SPI_INACTIVE, &channel);
		printf("soil moisture value is %u\n", received_value);
		
		if(received_value > 1000){
			write(sock,"3\n",2);
		}else{
			write(sock,"4\n",2);
		}
		sleep(3);
	}
	
	close(soilmois_dev);
	close(spi_dev);
	return NULL;
}

void * light_msg(void * arg){
    int sock = *((int*)arg);
    int fd;
    char buf[3];
    char val,value;
    float flight;
    fd=open("/dev/i2c-1",O_RDWR);
    if(fd<0)
    {
        printf("fd<0:%s\r\n",strerror(errno)); return (void*)1;
    }
    if(ioctl( fd,I2C_SLAVE,I2C_ADDR)<0 )
    {
        printf("SLAVE<0 : %s\r\n",strerror(errno));return (void*)1;
    }
    val=POWER_ON;
    if(write(fd,&val,1)<0)
    {
        printf("light sensor disconnect!\n");
    }
    val=CONTINUOS_HIGH_RES_MODE1;
    if(write(fd,&val,1)<0)
    {
        printf("light error\n");
    }
    while(1){
		if(read(fd,&buf,3)){
			flight=(buf[0]*256+buf[1])*0.5/1.2;
			printf("light's lux is %6.2flx\r\n",flight);
			if(flight>400.0){
				write(sock,"5\n",2);
			}else{
				write(sock,"6\n",2);
			}
			sleep(3);
		}	
	}	
	return NULL;
}
void error_handling(char * message){
	fputs(message,stderr);
	fputc('\n',stderr);
	exit(1);
}
