#define _CRT_SECURE_NO_WARNINGS
#define _REENTRANT
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/sysmacros.h>

#define LIGHT_MAJOR_NUMBER	501
#define LIGHT_MINOR_NUMBER	100
#define	LIGHT_DEV_NAME	"light"
#define LIGHT_DEV_PATH	"/dev/light"

#define IOCTL_MAGIC_NUMBER	'j'
#define IOCTL_CMD_LIGHT		_IOR(IOCTL_MAGIC_NUMBER, 0, int)
#define IOCTL_CMD_LOG		_IOW(IOCTL_MAGIC_NUMBER, 1, int)

#define INTERVAL	50000

int main(){
	int light_dev;
	pthread_t snd_thread, rcv_thread;
	void * thread_return;
		
	light_dev = makedev(LIGHT_MAJOR_NUMBER, LIGHT_MINOR_NUMBER);
	mknod(LIGHT_DEV_PATH, S_IFCHR | 0666, light_dev);
	
	light_dev = open(LIGHT_DEV_PATH, O_WRONLY);
	
	if(light_dev < 0){
		printf("fail to open light device driver\n");
		return -1;
	}
	while(1){
		int received;
		ioctl(light_dev, IOCTL_CMD_LIGHT, &received);
		sleep(1);
	}
	
	close(light_dev);
	return 0;
}
