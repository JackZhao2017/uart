#include "message.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "crc8.h"
#include "cmdqueue.h"



typedef struct{
	int p_byte;
	int p_bit;
}POSITION;

VEHICLESTATUS_INFO g_vehicleInfo;



void getVehiclestatusInfo(VEHICLESTATUS_INFO *vehicleInfo)
{
	memcpy(vehicleInfo,&g_vehicleInfo,sizeof(VEHICLESTATUS_INFO));
}

static void vehiclestatus_caculate(u16 *val)
{
	VEHICLESTATUS_INFO vehicle_info;
	
	vehicle_info.speed=(float)(val[0]*SPEED_CONEFFICIENT);
	printf("%s %x speed %f\n",__func__,val[0],vehicle_info.speed);
	vehicle_info.headlightstatus=val[1];
	vehicle_info.ldwenabled=val[2];
	vehicle_info.fcwenabled=val[3];
	memcpy(&g_vehicleInfo,&vehicle_info,sizeof(VEHICLESTATUS_INFO));
}

static void vehiclestatus_resolver(u8 *message)
{
	int i=0;	
	int t=0;
	unsigned short val[VEHICLESTATUS_VALID]={0};
	memset(val,0,sizeof(val));
	val[0]=((message[2]<<8)+message[3]);
	val[1]=(message[4]&0xf0)>>4;
	val[2]=(message[4]&0x08)?1:0;
	val[3]=(message[4]&0x04)?1:0;
	vehiclestatus_caculate(val);
}
static void syscontrol_rx_resolver(u8 *message)
{
	SYS_CTRLINFO   sysctrl_rx;
	memset(&sysctrl_rx,0,sizeof(sysctrl_rx));

	sysctrl_rx.Seqnum=message[2];
	sysctrl_rx.Commad=message[3];
	if(sysctrl_rx.Commad&0x80){
		sysctrl_rx.data=malloc(message[4]);
		memcpy(sysctrl_rx.data,&message[5],message[4]);
		sysctrl_rx.datalen=message[4];
	}
	putcmdintoQueue(sysctrl_rx);
	if(sysctrl_rx.data)
		free(sysctrl_rx.data);
}

static int crc8_detect(u8 *message)
{
	int packetsize=message[1];
	int crc=0;
	int val=0;
	crc=crc8(message,packetsize-1, 0);
	crc=crc8(message,packetsize-1,crc);
	if(crc!=message[packetsize-1])
		val=-1;
	return val;
}

int message_resolver(u8 *message)
{
	 int retval=0;
	 if(crc8_detect(message)<0){
	 	printf("%s crc8 not right\n",__func__);
	 	return -1;
	 }
	 switch(message[0])
	 {
	 	case VEHICLESTATUS:
	 		retval=VEHICLESTATUS;
	 		vehiclestatus_resolver(message);
	 		break;
	 	case SYSCONTROL_RX:
	 		retval=SYSCONTROL_RX;
	 		syscontrol_rx_resolver(message);
	 		break;
	 	default :
	 		break; 
	 }
	 return retval;
}

static int crc8_creator(u8 *m,int start,int len)
{
	u8 crc=0;
	crc=crc8(&m[start],len, 0);
	crc=crc8(&m[start],len, crc);
	return crc;
}


static void ldw_messagecreator(LDW_OUTINFO info,BUFINFO bufinfo)
{
	u16 temp=0,i;
	u8 message[LDW_MESSAGESIZE];
	//message=(u8 *)malloc(bufinfo.len);
	memset(message,0,LDW_MESSAGESIZE);

	message[0]=SYN_SIGN;
	message[1]=LDWSTATUS;
	message[2]=bufinfo.len-1;
	message[3]=(info.ldwCred&0xf)<<4+info.Errorcode&0xf;
	temp=(unsigned short)(info.ldwDis*0.01);
	message[4]=(temp>>8)&0xff;
	message[5]=temp&0xff;
	temp=(unsigned short)(info.ldwTime);
	message[6]=(temp>>8)&0xff;
	message[7]=temp&0xff;
	temp=(unsigned short)(info.CurveRadius);
	message[8]=(temp>>8)&0xff;
	message[9]=temp&0xff;
	temp=info.LaneWidth<<4+info.LaneType;
	message[10]=(temp>>8)&0xff;
	message[11]=temp&0xff;
	message[12]=crc8_creator(message,1,11);

	memcpy(bufinfo.addr,message,bufinfo.len);
	// free(message);
}

static void fcw_messagecreator(FCW_OUTINFO info,BUFINFO bufinfo)
{	
	int temp=0,i;
	// u8 *message=NULL;
	// message=malloc(bufinfo.len);
	u8 message[FCW_MESSAGESIZE];
	memset(message,0,FCW_MESSAGESIZE);

	message[0]=SYN_SIGN;
	message[1]=FCWSTATUS;
	message[2]=bufinfo.len-1;
	message[3]=(info.fcwCred&0xf)<<4+info.Errorcode&0xf;

	temp=(int)(info.fcwDis*100);
	message[4]=(temp>>8)&0xff;
	message[5]=temp&0xff;

	temp=(int)(info.fcwTtc*1000);

	message[6]=(temp>>8)&0xff;
	message[7]=temp&0xff;

	temp=(int)(info.fcwAttc*1000);
	message[8]=(temp>>8)&0xff;
	message[9]=temp&0xff;
	message[10]=crc8_creator(message,1,9);
	
	memcpy(bufinfo.addr,message,bufinfo.len);

	// free(message);
}
static void syscontrol_cmd_creator(SYS_CTRLINFO info,BUFINFO bufinfo)
{
	u8 *message=NULL;
	message=malloc(bufinfo.len);
	memset(message,0,bufinfo.len);

	message[0]=SYN_SIGN;
	message[1]=SYSCONTROL_TX;
	message[2]=bufinfo.len-1;
	message[3]=info.Seqnum;
	message[4]=info.Commad;
	if(info.datalen){
		message[5]=info.datalen;
		memcpy(&message[6],info.data,info.datalen);
	}
	message[bufinfo.len-1]=crc8_creator(message,1,bufinfo.len-2);
	memcpy(bufinfo.addr,message,bufinfo.len);
	free(message);
}
static void vehicle_messagecreator(VEHICLESTATUS_INFO info,BUFINFO bufinfo)
{
	u8 *message=NULL;
	u16 temp=0;
	float ftemp=0;
	message=malloc(bufinfo.len);
	memset(message,0,bufinfo.len);

	message[0]=SYN_SIGN;
	message[1]=VEHICLESTATUS;
	message[2]=bufinfo.len-1;
	ftemp=info.speed/SPEED_CONEFFICIENT;
	temp=(unsigned short)(ftemp);

	message[3]=(temp>>8)&0xff;

	message[4]=temp&0xff;
	message[5]=((info.headlightstatus<<4)&0xf0)|(info.ldwenabled?0x08:0x00)|(info.fcwenabled?0x04:0x00);
	message[6]=crc8_creator(message,1,5);
	memcpy(bufinfo.addr,message,bufinfo.len);
	
	free(message);
}
void message_creator(WARNNIG_CENTER center,int m,BUFINFO bufinfo)
{
	switch(m)
	{
	 	case SYSCONTROL_TX:
	 		syscontrol_cmd_creator(center.ctrl_info,bufinfo);
	 		break;
	 	case LDWSTATUS:
	 		ldw_messagecreator(center.ldw_info,bufinfo);
	 		break;
	 	case FCWSTATUS:
	 		fcw_messagecreator(center.fcw_info,bufinfo);

	 	case VEHICLESTATUS:
	 		vehicle_messagecreator(center.vehicle_info,bufinfo);
	 		break;
	}
}


