#include "ringbuffer.h"
#include "message.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void show_ringbufferinfo(RINGBUFFER *info)
{
	printf("putaddr:%d  getaddr %d  num:%d size:%d\n",
			info->putaddr,info->getaddr,info->num,info->size);
}

int ringbufferInit(RINGBUFFER *info,int size)
{
	info->data=(u8 *)malloc(size);
	memset(info->data,0,size);
	info->getaddr=0;
	info->putaddr=0;
	info->num=0;
	info->size=size;
}

int putdatatoBuffer(RINGBUFFER *info,u8 *buf,int len)
{
	if(len<=0||len>info->size-info->num)
		return -1;
	if(len<=info->size-info->putaddr){
	   memcpy(info->data+info->putaddr,buf,len);
	   info->putaddr=(info->putaddr+len)%info->size;
	}else{
	   memcpy(info->data+info->putaddr,buf,info->size-info->putaddr);
	   memcpy(info->data,buf+info->size-info->putaddr,len-(info->size-info->putaddr));
	   info->putaddr=len-(info->size-info->putaddr);
	}
	info->num+=len;
	return 0;
}

int getdatafromBuffer(RINGBUFFER *info,u8 *buf,int len)
{
	if(len>info->num||len<=0)
		return -1;
	if(len>info->size-info->getaddr){
		memcpy(buf,info->data+info->getaddr,info->size-info->getaddr);
		memcpy(buf+info->size-info->getaddr,info->data,len-(info->size-info->getaddr));
		info->getaddr=(info->getaddr+len)%info->size;
	}else{
		memcpy(buf,info->data+info->getaddr,len);
		info->getaddr+=len;
	}
	info->num-=len;
	return 0;
}

int addringaddr(RINGBUFFER *info)
{
	int addr=info->getaddr;
	info->num-=1;
	return ((addr+1)<RINGBUFSIZE)?(addr+1):0;
}
int detectSync(RINGBUFFER *info,u8 sync)
{
	int val=0;
	show_ringbufferinfo(info);
	if(info->num<=0)
		return 0;
	while(info->num>0){
		if(info->data[info->getaddr]==sync){
			val=1;
			break;
		}
		info->getaddr=addringaddr(info);
	}
	show_ringbufferinfo(info);
	return val;
}
int detectMsginfo(RINGBUFFER *info,int *len)
{
	int id =0,val=0;
	show_ringbufferinfo(info);
	if(info->num<3)
		return 0;
	info->getaddr=addringaddr(info);
	id=info->data[info->getaddr];
	switch(id){
		case VEHICLESTATUS:				
		case SYSCONTROL_RX:
			 val=id;
			 break;
		default:
		     val=0;
			 break;
	}
	*len=info->data[info->getaddr+1];
	if(info->num+1<*len)
		*len=0;
	show_ringbufferinfo(info);
	return  val;
}





















