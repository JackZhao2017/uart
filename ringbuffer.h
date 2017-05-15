#ifndef _RINGBUFFER_H_
#define _RINGBUFFER_H_

#ifdef __cplusplus
extern "C" {
#endif


#define RINGBUFSIZE 256
#define u8  unsigned char 
typedef struct{
	int   putaddr; //put first data index;
	int   getaddr; //get first data index;
	int   size;	 //buffer size;
	int   num;     //vaild data len;
	u8   *data;
}RINGBUFFER;

int ringbufferInit(RINGBUFFER *info,int size);
int putdatatoBuffer(RINGBUFFER *info,char *buf,int len);
int getdatafromBuffer(RINGBUFFER *info,char *buf,int len);
int addringaddr(int addr);
int detectSync(RINGBUFFER *info,u8 sync);
void show_ringbufferinfo(RINGBUFFER *info);
int isprocessMsg(RINGBUFFER *info,int *len);




#ifdef __cplusplus
};
#endif

#endif


