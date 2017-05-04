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
VEHICLESTATUS_INFO g_mVehicleInfo;
int main(int argc ,char **argv)
{
	int count=0,ret,len=13;
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
		// memset(buf,0,sizeof(buf));		
		char buf[]={0x30,0xd,0xc8,0x00,0x18,0x64,0x3d,0xef,0x1e,0x1,0x2,0x3,0x4};//
		while(issendBusy()){
			if(++count>3)
				break;
		}
		uartsendData(buf ,len);
		sleep(1);
		getVehiclestatusInfo(&g_mVehicleInfo);
		printf("speed :            %f \n"\
	   		   "steeringangle      %f \n"\
	   			"steeringanglevalid %d \n"\
	   			"leftturnon         %d \n"\
	   			"rightturnon        %d \n"\
	   			"frontwiperlevel    %d \n"\
	   			"headlightstatus    %d \n"\
	   			"ldwenabled         %d \n"\
	   			"fcwenabled         %d \n"\
	   			"ldwsensitivity     %d \n"\
	   			"fcwsensitivity     %d \n"\
	   			"breaklevel         %d \n"\
	   			"gearshift          %d \n",
	   g_mVehicleInfo.speed,g_mVehicleInfo.steeringangle,g_mVehicleInfo.steeringanglevalid,g_mVehicleInfo.leftturnon,g_mVehicleInfo.rightturnon,g_mVehicleInfo.frontwiperlevel,
	   g_mVehicleInfo.headlightstatus,g_mVehicleInfo.ldwenabled,g_mVehicleInfo.fcwenabled,g_mVehicleInfo.ldwsensitivity,g_mVehicleInfo.fcwsensitivity,g_mVehicleInfo.breaklevel,
	   g_mVehicleInfo.gearshift
	  );
	}
	uartRelease();
	return 0;
}
