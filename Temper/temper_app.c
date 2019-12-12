#define _CRT_SECURE_NO_WARNINGS
#define _REENTRANT
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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

#define TEMPER_MAJOR_NUMBER	504
#define TEMPER_MINOR_NUMBER	100
#define	TEMPER_DEV_NAME	"temper"
#define TEMPER_DEV_PATH	"/dev/temper"

#define IOCTL_MAGIC_NUMBER	'j'
#define IOCTL_CMD_TEMPER	_IOR(IOCTL_MAGIC_NUMBER, 0, int)
#define IOCTL_CMD_LOG		_IOW(IOCTL_MAGIC_NUMBER, 1, int)

#define INTERVAL	50000

#define BUF_SIZE 100
#define NAME_SIZE 20
	
void * send_msg(void * arg);
void error_handling(char * msg);

int temper_dev;

char name[NAME_SIZE]="[DEFAULT]";
char msg[BUF_SIZE];

int main(int argc, char * argv[]){
	int sock;
	struct sockaddr_in serv_addr;
	pthread_t snd_thread;
	void * thread_return;
	
	if(argc!=3){
		printf("Usage : %s <IP> <port>\n",argv[0]);
		exit(1);
	}
	
	sock=socket(PF_INET, SOCK_STREAM, 0);
	
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family=AF_INET;
	serv_addr.sin_addr.s_addr=inet_addr(argv[1]);
	serv_addr.sin_port=htons(atoi(argv[2]));
	
	if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))==-1)
		error_handling("connect() error");
		
		
	temper_dev = makedev(TEMPER_MAJOR_NUMBER, TEMPER_MINOR_NUMBER);
	mknod(TEMPER_DEV_PATH, S_IFCHR | 0666, temper_dev);
	
	temper_dev = open(TEMPER_DEV_PATH, O_WRONLY);
	
	if(temper_dev < 0){
		printf("fail to open temper device driver\n");
		return -1;
	}
	
	pthread_create(&snd_thread, NULL, send_msg, (void*)&sock);
	pthread_join(snd_thread, &thread_return);
	close(sock);
	close(temper_dev);
	return 0;
}

void *send_msg(void * arg){  
	int sock = *((int*)arg);
	char name_msg[BUF_SIZE];

	char data[30];
	double high,low;
	while(1){
		high=0,low=0;
		memset(data,0,sizeof(data));
		ioctl(temper_dev, IOCTL_CMD_TEMPER, data);
		printf("%s\n",data);
		high = (double)(data[0]-'0')*10;
		high += (double)(data[1]-'0');
		low = (double)(data[3]-'0');
		high += (0.1*low);
		printf("%lf\n",high);
		if(high>30.0){
			printf("1\n");
			write(sock,"1\n",2);
		}else{
			printf("2\n");
			write(sock,"2\n",2);
		}
		sleep(3);
	}
	return NULL;
}

void error_handling(char * message){
	fputs(message,stderr);
	fputc('\n',stderr);
	exit(1);
}
