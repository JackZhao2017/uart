#include "ringbuffer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>




int ringbufferInit(RINGBUFFER *info,int size)
{
	info->data=malloc(size);
	memset(info->data,0,size);
	info->getaddr=0;
	info->putaddr=0;
	info->num=0;
	info->size=size;
}

int putdatatoBuffer(RINGBUFFER *info,char *buf,int len)
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

int getdatafromBuffer(RINGBUFFER *info,char *buf,int len)
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
int isprocessMsg(RINGBUFFER *info,int *len)
{
	if(info->num<7)
		return 0;
	
	return 	1;
}
