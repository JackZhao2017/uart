#include "message.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "crc8.h"


typedef struct{
	int p_byte;
	int p_bit;
}POSITION;

VEHICLESTATUS_INFO g_vehicleInfo;
SYS_CTRLINFO 	   g_sysctrl_rx;


void getVehiclestatusInfo(VEHICLESTATUS_INFO *vehicleInfo)
{
	memcpy(vehicleInfo,&g_vehicleInfo,sizeof(VEHICLESTATUS_INFO));
}

void vehiclestatus_caculate(int *val)
{
	VEHICLESTATUS_INFO vehicle_info;
	
	vehicle_info.speed=(float)(val[0]*SPEED_CONEFFICIENT);
	printf("%s  spped %f\n",__func__,vehicle_info.speed);
	vehicle_info.headlightstatus=val[1];
	vehicle_info.ldwenabled=val[2];
	vehicle_info.fcwenabled=val[3];
	memcpy(&g_vehicleInfo,&vehicle_info,sizeof(VEHICLESTATUS_INFO));
}

void getbit(char *m,int *byte,int *bit,int num,int *val)
{
	int ch=0;
	int b =0;
	int i=0;
	int temp=0;
	char data=(0x80>>b);
	ch=*byte;
	b =*bit;
	printf("%s  %d \n",__func__,ch);
	for(i=0;i<num;i++)
	{
		if(m[ch]&data)
			temp=(temp|0x1)<<1;
		else
			temp=temp<<1;
		data>>=1;
		printf("%x \n",temp);
		if(data==0){
			data=0x80;
			ch+=1;
		}
	}
	*val=temp;
	*byte=ch;
	*bit=b+num-1;
	if(*bit>7)
		*bit-=7;
}

void vehiclestatus_resolver(char *message)
{
	int i=0;	
	int t=0;
	int val[VEHICLESTATUS_VALID]={0};
	memset(val,0,sizeof(val));
	val[0]=(message[2]<<8)+message[3];
	val[1]=(message[4]&0x80)?1:0;
	val[2]=(message[4]&0x40)?1:0;
	val[3]=(message[4]&0x20)?1:0;
	vehiclestatus_caculate(val);
}
void cleanCommand(void)
{
	memset(&g_sysctrl_rx,0,sizeof(g_sysctrl_rx));
}
int  getCommand(int *cmd)
{
	*cmd=g_sysctrl_rx.Commad;
	return g_sysctrl_rx.Seqnum;
}

void syscontrol_rx_resolver(char *message)
{
	memset(&g_sysctrl_rx,0,sizeof(g_sysctrl_rx));

	g_sysctrl_rx.Seqnum=message[2];
	g_sysctrl_rx.Commad=message[3];
	if(g_sysctrl_rx.Commad&0x80){
		g_sysctrl_rx.data=malloc(message[4]);
		memcpy(g_sysctrl_rx.data,&message[5],message[4]);
		g_sysctrl_rx.datalen=message[4];
	}
}

int crc8_detect(char *message)
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

int message_resolver(char *message)
{
	 int retval=0;
	 if(crc8_detect(message)<0){
	 	printf("%s crc8 not correct\n",__func__);
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

int crc8_creator(char *m,int start,int len)
{
	char crc=0;
	crc=crc8(&m[start],len, 0);
	crc=crc8(&m[start],len, crc);
	return crc;
}


void ldw_messagecreator(LDW_OUTINFO info,BUFINFO bufinfo)
{
	unsigned short temp=0,i;
	char *message=NULL;
	message=malloc(bufinfo.len);
	memset(message,0,bufinfo.len);

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
	free(message);
}

void fcw_messagecreator(FCW_OUTINFO info,BUFINFO bufinfo)
{	
	int temp=0,i;
	char *message=NULL;

	message=malloc(bufinfo.len);
	memset(message,0,bufinfo.len);

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

	free(message);
}
void syscontrol_cmd_creator(SYS_CTRLINFO info,BUFINFO bufinfo)
{
	char *message=NULL;
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
void vehicle_messagecreator(VEHICLESTATUS_INFO info,BUFINFO bufinfo)
{
	char *message=NULL;
	int temp=0;
	float ftemp=0;
	message=malloc(bufinfo.len);
	memset(message,0,bufinfo.len);

	message[0]=SYN_SIGN;
	message[1]=VEHICLESTATUS;
	message[2]=bufinfo.len-1;
	ftemp=info.speed/SPEED_CONEFFICIENT;
	temp=(int)(ftemp);
	message[3]=(temp>>8)&0xff;
	message[4]=temp&0xff;
	message[5]=(info.headlightstatus?0x80:0x00)|(info.ldwenabled?0x40:0x00)|(info.fcwenabled?0x20:0x00);
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


