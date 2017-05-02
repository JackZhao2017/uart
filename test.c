#include "uart.h"
#include "message.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
int ctrl_c_rev = 1;
void ctrl_c_handler(int signum, siginfo_t *info, void *myact)
{
	ctrl_c_rev = 0;
	return;
}

int main(int argc ,char **argv)
{
	int count=0,ret,len;
	uartInit(argc,argv);
	struct sigaction act;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_SIGINFO;
	act.sa_sigaction = ctrl_c_handler;
	if((ret = sigaction(SIGINT, &act, NULL)) < 0) {
			printf("install sigal error\n");
			return -1;
	}
	while(ctrl_c_rev)
	{
		// char buf[125]={0};		
		// sprintf(buf,"this time send buf\r\n");	
		// len=strlen(buf);
		// while(iswriteBusy()){
		// 	if(++count>3)
		// 		break;
		// }
		// uartsendData(buf ,len);
		sleep(1);
	}
	uartRelease();
	return 0;
}
