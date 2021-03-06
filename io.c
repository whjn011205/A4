#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
//needed for IO things. Attention that this is different from kernel mode int lcd;

#define SCULL_IOC_MAGIC 'k'
#define SCULL_HELLO _IO(SCULL_IOC_MAGIC, 1)
#define SET_DEV_MSG _IOW(SCULL_IOC_MAGIC, 2, char*)
#define GET_DEV_MSG _IOR(SCULL_IOC_MAGIC, 3, char*)
#define WR_DEV_MSG _IOWR(SCULL_IOC_MAGIC, 4, char*)

int lcd;

void test() 
{ 
	int k, i, sum;
	char s[3];
	
	memset(s, '2', sizeof(s));
	
	printf("test begin!\n");
	k = write(lcd, s, sizeof(s));

	printf("written = %d\n", k);
	k = ioctl(lcd, SCULL_HELLO);
	
	printf("result = %d\n", k);

	char *pre_msg = "Old message\n";
	if(k = ioctl(lcd, SET_DEV_MSG, pre_msg)){
		printf("ioctl set message fail\n");
		return ;
	} 

	char *user_msg = (char *)malloc(60);
	if(k = ioctl(lcd, GET_DEV_MSG, user_msg)){
		printf("_IOR ioctl get message fail\n");
		return ;
	}
	printf("user_msg: %s\n", user_msg);  


	char *new_user_msg = "New message!\n";
	strcpy(user_msg, new_user_msg);
	if(k = ioctl(lcd, WR_DEV_MSG, user_msg)){
		printf("_IOWR ioctl exchange message fail\n");
		return ;
	}
	printf("user_msg: %s \n", user_msg); 
	free(user_msg); 
}

int main(int argc, char **argv)
{
	lcd = open("/dev/four", O_RDWR);
	
	if(lcd == -1){
		perror("unable to open lcd");
		exit(EXIT_FAILURE);	
	}
	
	test();
	close(lcd);
	return 0;
}

