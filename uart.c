#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <malloc.h>

#include <pthread.h>
#include <semaphore.h>

#include "message.h"
#include "uart.h"
#include "ringbuffer.h" 

#define DEFAULT_RATE 115200;
static speed_t baudrate_map(unsigned long b)
{
    speed_t retval;

    switch(b)
    {
        case 110:
            retval = B110;
            break;

        case 300:
            retval = B300;
            break;

        case 1200:
            retval = B1200;
            break;

        case 2400:
            retval = B2400;
            break;

        case 4800:
            retval = B4800;
            break;

        case 9600:
            retval = B9600;
            break;

        case 19200:
            retval = B19200;
            break;

        case 38400:
            retval = B38400;
            break;

        case 57600:
            retval = B57600;
            break;

        case 115200:
            retval = B115200;
            break;
    }
    return(retval);
}

#define TRUE  1
#define FALSE 0
char 		*g_uartDev="/dev/ttymxc2";
int  		g_fd_uart=0;
pthread_t 	p_Uartsend, p_Uartread;
int 		g_rrun ,g_trun;

sem_t 		g_sem_tx,g_sem_rx;

char *g_txbuf=NULL;
int  g_txlen;
int  g_isTxfinished=FALSE;
char *g_rxbuf=NULL;
int  g_rxlen;
int  g_isRxfinished=FALSE;

RINGBUFFER g_ringbufInfo;

int readdataComplete(int val)
{
	g_isRxfinished=TRUE;
	return g_isRxfinished;
}

static int isprocessMessage(int *size)
{
	int i=0;
	int retval=0;
	int syn_flash=0;
	int packet_flash=0;
	int packetsize=0;
	int detectsize=g_ringbufInfo.num;
	int startaddr =g_ringbufInfo.getaddr;
	show_ringbufferinfo(&g_ringbufInfo);
	if(detectsize<7)
		return 0;
	for(i=0;i<detectsize;i++)
	{
		int ind=i+startaddr;
		g_ringbufInfo.num-=1;
		if(ind>RINGBUFSIZE-1)
			ind-=RINGBUFSIZE;
		g_ringbufInfo.getaddr=ind;	
		if(g_ringbufInfo.data[ind]==SYN_SIGN){
			syn_flash=1;
			continue;
		}
		if(syn_flash){
			switch(g_ringbufInfo.data[ind])
			{
				case VEHICLESTATUS:				
				case SYSCONTROL_RX:
					 packet_flash=1;
					 break;
				default:
					 packet_flash=0;
					 break;
			}
			syn_flash=0;
			if(packet_flash)
				continue;
		}
		if(packet_flash){
			packetsize=g_ringbufInfo.data[ind];
			if(g_ringbufInfo.num<packetsize-2){				
				g_ringbufInfo.num+=3;
				g_ringbufInfo.getaddr=ind-2;
				retval=0;
			}
			else{
				g_ringbufInfo.num+=2;
				g_ringbufInfo.getaddr=ind-1;
				*size = packetsize;				
				retval=1;
			}
			break;
		}
		
	}
	show_ringbufferinfo(&g_ringbufInfo);
	return retval;
}

static void *uartRead(void * threadParameter)
{
	u8 data[RINGBUFSIZE];
	int iores, iocount=0,len=0,isSync=0;
	int old =0;
	printf("uartRead thread \n");
	while(g_rrun) { 
		iores = ioctl(g_fd_uart, FIONREAD, &iocount);
		if(iocount){
			memset(data,0,sizeof(data));
        	iores = read(g_fd_uart, data, iocount);
        	printf("iocount %d \n",iocount );
        	putdatatoBuffer(&g_ringbufInfo,data,iocount);
        	if(isprocessMessage(&len)){
 				memset(data,0,sizeof(data));
        		getdatafromBuffer(&g_ringbufInfo,data,len);
        		for(iocount=0;iocount<len;iocount++)
        		   printf("0x%x ",data[iocount]);
        		   printf("\n");
        		message_resolver(data);
        	}	 	
		}
	}
	pthread_exit(NULL);
}


int issendBusy()
{
	return g_isTxfinished;
}

int uartsendData(char *buf ,int len)
{
	if(g_txbuf)
		free(g_txbuf);
	g_txbuf=malloc(len);
	memcpy(g_txbuf,buf,len);
	g_txlen=len;
	sem_post(&g_sem_tx);
	return 0;
}

static void *uartWrite(void * threadParameter)
{
	printf("uartSend thread \n");
	while(g_trun)
	{
		sem_wait(&g_sem_tx);
		g_isTxfinished=FALSE;
		write(g_fd_uart,g_txbuf,g_txlen);
		g_isTxfinished=TRUE;
	}
	pthread_exit(NULL);
}

int uartInit(int argc ,char **argv)
{
	struct termios options;
	unsigned long baudrate = DEFAULT_RATE;
	int ret,i;

	printf("%s argc %d   %s\n",__func__,argc,g_uartDev);
	 g_fd_uart = open(g_uartDev, O_RDWR | O_NOCTTY);
	if (g_fd_uart == -1) {
		printf("open_port: Unable to open serial port - %s", g_uartDev);
		return -1;
	}
	fcntl(g_fd_uart, F_SETFL, 0);
	tcgetattr(g_fd_uart, &options);

	options.c_cflag &= ~CSTOPB;

	options.c_cflag &= ~CSIZE;
	options.c_cflag |= PARENB;
	options.c_cflag &= ~PARODD;
	options.c_cflag |= CS8;
	options.c_cflag &= ~CRTSCTS;

	options.c_lflag &= ~(ICANON | IEXTEN | ISIG | ECHO| ECHONL);
	options.c_oflag &= ~OPOST;
	options.c_iflag &= ~(ICRNL | INPCK | ISTRIP | IXON | BRKINT );

	options.c_cc[VMIN] = 1;
	options.c_cc[VTIME] = 0;
	options.c_cflag |= (CLOCAL | CREAD);

	for(i = 1; i < argc; i++) {
		
		if (!strcmp(argv[i], "-S")) {
			options.c_cflag |= CSTOPB;
			continue;
		}
		if (!strcmp(argv[i], "-O")) {
			options.c_cflag |= PARODD;
			options.c_cflag &= ~PARENB;
			continue;
		}
		if (!strcmp(argv[i], "-E")) {
			options.c_cflag &= ~PARODD;
			options.c_cflag |= PARENB;
			continue;
		}
		if (!strcmp(argv[i], "-HW")) {
			options.c_cflag |= CRTSCTS;
			continue;
		}
		if (!strcmp(argv[i], "-B")) {
			i++;
			baudrate = atoi(argv[i]);
			if(!baudrate_map(baudrate))
				baudrate = DEFAULT_RATE;
			continue;
		}
	}

	if(baudrate) {
		cfsetispeed(&options, baudrate_map(baudrate));
		cfsetospeed(&options, baudrate_map(baudrate));
	}

	tcsetattr(g_fd_uart, TCSANOW, &options);
	printf("UART %lu, %dbit, %dstop, %s, HW flow %s\n", baudrate, 8,
	       (options.c_cflag & CSTOPB) ? 2 : 1,
	       (options.c_cflag & PARODD) ? "PARODD" : "PARENB",
	       (options.c_cflag & CRTSCTS) ? "enabled" : "disabled");

	g_rrun = 1;
	g_trun = 1;

	memset(&g_ringbufInfo,0,sizeof(g_ringbufInfo));
	ringbufferInit(&g_ringbufInfo,RINGBUFSIZE);

	

	ret = pthread_create(&p_Uartread, NULL, uartRead, NULL);
	if(ret<0)
	{
		printf("failed to create uart read thread \n");
		return -1;
	}
	ret = pthread_create(&p_Uartsend, NULL, uartWrite, NULL);
	if(ret<0)
	{
		printf("failed to create uart send thread \n");
		return -1;
	}
	sem_init(&g_sem_rx,0,0);
	sem_init(&g_sem_tx,0,0);
	printf("uart initilizate successful \n");
	return 0;
}

void uartRelease()
{
	int ret;
	g_rrun = 0;
	sem_post(&g_sem_rx);
	ret = pthread_join(p_Uartread, NULL);
	if(ret < 0)
		printf("fail to stop Uartread thread\n");
	g_trun = 0;
	sem_post(&g_sem_tx);
	ret = pthread_join(p_Uartsend, NULL);
	if(ret < 0)
		printf("fail to stop Uartsend thread\n");
	close(g_fd_uart);
	printf("%s\n",__func__ );
}

