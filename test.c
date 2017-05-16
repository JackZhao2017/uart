#include "uart.h"
#include "message.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include "crc8.h"
#include "capturetimer.h"
#include "cmdqueue.h"
int ctrl_c_rev = 1;
void ctrl_c_handler(int signum, siginfo_t *info, void *myact)
{
	ctrl_c_rev = 0;
	return;
}
VEHICLESTATUS_INFO g_mVehicleInfo;
int main(int argc ,char **argv)
{
	char buf[]={0x55,0x30,0x6,0x19,0x00,0x60,0x21};//
	char cmd[]={0x55,0x20,0x5,0x1,0x10,0x14};
	static int count=0;
	int ret,len=7;
	int commad=0,seqnum=0;
	uartInit(argc,argv);
	struct sigaction act;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_SIGINFO;
	act.sa_sigaction = ctrl_c_handler;
	if((ret = sigaction(SIGINT, &act, NULL)) < 0) {
			printf("install sigal error\n");
			return -1;
	}
	if(capture_timeinit(2)<0)
    {
          printf("<%s>:capture timer initialize failed \n",__func__);
          return -1;
    }
	crcInit(LSB,POLY);
	WARNNIG_CENTER center;
	SYS_CTRLINFO   syscmd;
	memset(&syscmd,0,sizeof(syscmd));
	memset(&center,0,sizeof(center));
	while(ctrl_c_rev)
	{	
		wait_timersignal();
		static float speed=1;

		BUFINFO bufinfo;
		bufinfo.len=sizeof(buf);
		bufinfo.addr=&buf[0];
		if(speed==360)
			speed=1;
		center.vehicle_info.speed=speed;
		printf("\n%s speed %f \n",__func__,speed++);
		center.vehicle_info.headlightstatus=4;
		center.vehicle_info.ldwenabled=1;
		center.vehicle_info.fcwenabled=1;
		message_creator(center,VEHICLESTATUS,bufinfo);
		uartsendData(bufinfo.addr ,bufinfo.len);
		usleep(100);
		while(issendBusy());
		uartsendData(cmd,sizeof(cmd));
		
		getVehiclestatusInfo(&g_mVehicleInfo);
		if(iscmdneedProcess()){
			getcmdfromQueue(&syscmd);
			printf("\nseqnum: %d \n"
				   "commad   %d \n",
				   syscmd.Seqnum,syscmd.Commad
				  );
		}
		if(g_mVehicleInfo.ldwenabled)
			printf("\nspeed :            %f \n"	   		   
	   			"headlightstatus    %d \n"
	   			"ldwenabled         %d \n"
	   			"fcwenabled         %d \n",
	    		g_mVehicleInfo.speed,g_mVehicleInfo.headlightstatus,g_mVehicleInfo.ldwenabled,g_mVehicleInfo.fcwenabled
	  		);
	}
	uartRelease();
	return 0;
}
