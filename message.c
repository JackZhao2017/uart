#include "message.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VEHICLESTATUS_VALID 15
#define SYSCONTROL_RX_VALID  4

#define SPEED_CONEFFICIENT 0.05625
#define STEERINGANGLE_CONEFFICIENT 0.1





char syscontrol[SYSCONTROL_RX_VALID]={8,8,8,8}; 
typedef struct{
	int p_byte;
	int p_bit;
}POSITION;
VEHICLESTATUS_INFO g_vehicleInfo;
#define printf 
void getVehiclestatusInfo(VEHICLESTATUS_INFO *vehicleInfo)
{
	memcpy(vehicleInfo,&g_vehicleInfo,sizeof(VEHICLESTATUS_INFO));
}
void vehiclestatus_caculate(int *val)
{
	VEHICLESTATUS_INFO vehicle_info;
	vehicle_info.speed=val[2]*SPEED_CONEFFICIENT;
	vehicle_info.steeringangle=val[3]*STEERINGANGLE_CONEFFICIENT;
	vehicle_info.steeringanglevalid=val[4];
	vehicle_info.leftturnon=val[5];
	vehicle_info.rightturnon=val[6];
	vehicle_info.frontwiperlevel=val[7];
	vehicle_info.headlightstatus=val[8];
	vehicle_info.ldwenabled=val[9];
	vehicle_info.fcwenabled=val[10];
	vehicle_info.ldwsensitivity=val[11];
	vehicle_info.fcwsensitivity=val[12];
	vehicle_info.breaklevel=val[13];
	vehicle_info.gearshift=val[14];
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
	int val[VEHICLESTATUS_VALID]={0};
	printf("%s :\n",__func__);
	val[2]=(message[2]<<5)+(message[3]>>3);
	printf("%x \n",val[2]);
	val[3]=(message[3]<<13)+(message[4]<<5)+(message[5]>>3);
	printf("%d \n",val[3]);
	//SteeringAngleValid
	val[4]=message[5]&(0x4)?1:0;
	//TurnLightOn
	val[5]=message[5]&(0x2)?1:0;
	val[6]=message[5]&(0x1)?1:0;
	printf("%x  %x  %x \n",val[4],val[5],val[6]);
	//FrontWiperLevel
	val[7]=(message[6]&(0xf0))>>4;
	//HeadLightStatus
	val[8]=message[6]&(0x8)?1:0;
	//LDWEnabled
	val[9]=message[6]&(0x4)?1:0;
	//FCWEnabled
	val[10]=message[6]&(0x2)?1:0;

	printf("%x  %x  %x  \n",val[7],val[8],val[9],val[10]);
	//LDWSensitivity;
	val[11]=((message[6]<<3)+(message[7]>>5))&0xf;   

	printf("%x   \n",val[11]);
	//FCWSensitivity
	val[12]=(message[7]&0x1e)>>1; ;
	//BreakLevel
	val[13]=((message[7]&0x01)<<3)+(message[8]>>5);         
	//GearShift
	val[14]=(message[8]&0x1e)>>1;           
	printf("%x  %x  %x \n",val[12],val[13],val[14]);
	vehiclestatus_caculate(val);
}

void syscontrol_rx_resolver(char *message)
{
	int seqnum=message[2];
	int commad=message[3];	
}

int crc8_err(char *message)
{
	int packetsize=message[1];
	int crc=0;
	int i=0;

	return 0;
}

int message_resolver(char *message)
{
	 int retval=0;

	 if(crc8_err(message)<0)
	 	return -1;

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




void ldw_messagecreator(LDW_OUTINFO info,BUFINFO bufinfo)
{
	unsigned short temp=0,i;
	int crc=0x01020304;
	char *message=NULL;
	printf("%s: \n",__func__);
	message=malloc(bufinfo.len);
	memset(message,0,bufinfo.len);

	message[0]=LDWSTATUS;
	message[1]=bufinfo.len;
	message[2]=(info.ldwCred&0xf)<<4;
	temp=(unsigned short)(info.ldwDis*0.01);
	message[2]|=(temp&0xf000)>>12;
	message[3]=(temp&0x0ff0)>>4;
	message[4]=(temp&0xf)<<4;
	temp=(unsigned short)(info.ldwV);
	message[4]|=(temp&0xf000)>>12;
	message[5]=(temp&0xff0)>>4;
	message[6]=(temp&0xf)<<4;

	memcpy(bufinfo.addr,message,bufinfo.len);
	free(message);
}

void fcw_messagecreator(FCW_OUTINFO info,BUFINFO bufinfo)
{	
	int temp=0,i;
	int crc=0x01020304;
	char *message=NULL;
	printf("%s: \n",__func__);
	message=malloc(bufinfo.len);
	memset(message,0,bufinfo.len);

	message[0]=FCWSTATUS;
	message[1]=bufinfo.len;
	message[2]=(info.fcwCred&0xf)<<4;
	temp=(int)(info.fcwDis*100);
	message[2]|=(temp&0xf0000)>>16;
	message[3]=(temp&0xff00)>>8;
	message[4]=temp&0xff;
	temp=(int)(info.fcwTtc*1000);
	message[5]=(temp&0xff00)>>8;
	message[6]=(temp&0xff);
	temp=(int)(info.fcwAttc*1000);
	message[7]=(temp&0xff00)>>8;
	message[8]=(temp&0xff);

	memcpy(bufinfo.addr,message,bufinfo.len);

	free(message);
}
void syscontrol_cmd_creator()
{
	

}
void message_creator(WARNNIG_CENTER center,int m,BUFINFO bufinfo)
{
	printf("%s: \n",__func__);
	switch(m)
	{
	 	case SYSCONTROL_TX:
	 		syscontrol_cmd_creator();
	 		break;
	 	case LDWSTATUS:
	 		ldw_messagecreator(center.ldw_info,bufinfo);
	 		break;
	 	case FCWSTATUS:
	 		fcw_messagecreator(center.fcw_info,bufinfo);
	 		break;
	}

}
 
