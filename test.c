#include "uart.h"
#include "message.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "crc8.h"
int ctrl_c_rev = 1;
void ctrl_c_handler(int signum, siginfo_t *info, void *myact)
{
	ctrl_c_rev = 0;
	return;
}
VEHICLESTATUS_INFO g_mVehicleInfo;
int main(int argc ,char **argv)
{
	int count=0,ret,len=7;
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
	crcInit(LSB,POLY);
	WARNNIG_CENTER center;
	memset(&center,0,sizeof(center));
	while(ctrl_c_rev)
	{	
		char crc=0;
		char buf[]={0x55,0x30,0x6,0x19,0x00,0xe0,0x3d};//
		char cmd[]={0x55,0x20,0x5,0x1,0x10,0x3d};
		BUFINFO bufinfo;
		bufinfo.len=sizeof(buf);
		bufinfo.addr=malloc(bufinfo.len);
		center.vehicle_info.speed=360;
		center.vehicle_info.headlightstatus=0;
		center.vehicle_info.ldwenabled=1;
		center.vehicle_info.fcwenabled=1;
		message_creator(center,VEHICLESTATUS,bufinfo);
		uartsendData(bufinfo.addr ,bufinfo.len);
		free(bufinfo.addr);
		usleep(100);
		
		len=sizeof(cmd);
		crc=crc8(&cmd[1],len-2,0);
		cmd[len-1]=crc8(&cmd[1],len -2,crc);
		uartsendData(cmd ,len);
		sleep(1);

		getVehiclestatusInfo(&g_mVehicleInfo);
		seqnum=getCommd(&commad);
		printf("\nspeed :            %f \n"	   		   
	   			"headlightstatus    %d \n"
	   			"ldwenabled         %d \n"
	   			"fcwenabled         %d \n",
	    		g_mVehicleInfo.speed,g_mVehicleInfo.headlightstatus,g_mVehicleInfo.ldwenabled,g_mVehicleInfo.fcwenabled
	  	);
	  	printf("seqnum :  %d   commad : %d \n",seqnum,commad);
	}
	uartRelease();
	return 0;
}
