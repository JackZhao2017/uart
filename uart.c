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

#ifdef B230400
        case 230400:
            retval = B230400;
            break;
#endif

#ifdef B460800
        case 460800:
            retval = B460800;
            break;
#endif

#ifdef B500000
        case 500000:
            retval = B500000;
            break;
#endif

#ifdef B576000
        case 576000:
            retval = B576000;
            break;
#endif

#ifdef B921600
        case 921600:
            retval = B921600;
            break;
#endif

#ifdef B1000000
        case 1000000:
            retval = B1000000;
            break;
#endif

#ifdef B1152000
        case 1152000:
            retval = B1152000;
            break;
#endif

#ifdef B1500000
        case 1500000:
            retval = B1500000;
            break;
#endif

#ifdef B2000000
        case 2000000:
            retval = B2000000;
            break;
#endif

#ifdef B2500000
        case 2500000:
            retval = B2500000;
            break;
#endif

#ifdef B3000000
        case 3000000:
            retval = B3000000;
            break;
#endif

#ifdef B3500000
        case 3500000:
            retval = B3500000;
            break;
#endif

#ifdef B4000000
        case 4000000:
            retval = B4000000;
            break;
#endif

        default:
            retval = 0;
            break;
    }
    return(retval);
}

#define TRUE  1
#define FALSE 0
char 		*g_uartDev="/dev/ttymxc4";
int  		g_fd_uart=0;
pthread_t 	p_Uartsend, p_Uartread;
int 		g_rrun ,g_trun;

sem_t 		g_sem_tx,g_sem_rx;

char *g_txbuf=NULL;
int  g_txlen;
int  g_isTxfinished=0;
char *g_rxbuf=NULL;
int  g_rxlen;
int  g_isRxfinished=0;

char *ringbuf=NULL;
#define  BUFSIZE 256
#define  MINSIZE 13

typedef struct 
{
	int storageaddr;
	int processaddr;
	int canstoragelen;
	int canprocesslen;
}RINGBUF_INFO;

RINGBUF_INFO g_ringbufinfo;

int readdataComplete(int val)
{
	g_isRxfinished=TRUE;
	return g_isRxfinished;
}
int getuartData(char **buf,int *len)
{
	sem_wait(&g_sem_rx);
	*buf=g_rxbuf;
	*len=g_rxlen;
	return 0;
}
static int readPost(char *buf ,int len)
{
	g_rxbuf=malloc(len);
	g_rxlen=len;
	memcpy(g_rxbuf,buf,len);
	sem_post(&g_sem_rx);
	return 0;
}
static int storagetoRingbuf(char *buf,int len)
{

	if((BUFSIZE-g_ringbufinfo.storageaddr)<len){
		memcpy(&ringbuf[g_ringbufinfo.storageaddr],buf,BUFSIZE-g_ringbufinfo.storageaddr);
		memcpy(ringbuf,&buf[BUFSIZE-g_ringbufinfo.storageaddr],len-BUFSIZE+g_ringbufinfo.storageaddr);
		g_ringbufinfo.storageaddr=len-BUFSIZE+g_ringbufinfo.storageaddr;
	}
	else{
		memcpy(&ringbuf[g_ringbufinfo.storageaddr],buf,len);
		g_ringbufinfo.storageaddr+=len;
		if(g_ringbufinfo.storageaddr==BUFSIZE)
			g_ringbufinfo.storageaddr=0;
	}
	g_ringbufinfo.canstoragelen-=len;
	g_ringbufinfo.canprocesslen+=len;
	return 0;
}
static int getfromRingbuf(char *buf,int len)
{
	if(g_ringbufinfo.processaddr+len>BUFSIZE){
		memcpy(buf,&ringbuf[g_ringbufinfo.processaddr],BUFSIZE-g_ringbufinfo.processaddr);
		memcpy(&buf[BUFSIZE-g_ringbufinfo.processaddr],ringbuf,len-BUFSIZE+g_ringbufinfo.processaddr);
		g_ringbufinfo.processaddr=len-BUFSIZE+g_ringbufinfo.processaddr;
	}else{
		memcpy(buf,&ringbuf[g_ringbufinfo.processaddr],len);
		g_ringbufinfo.processaddr+=len;
		if(g_ringbufinfo.processaddr==BUFSIZE)
			g_ringbufinfo.processaddr=0;
	}
	g_ringbufinfo.canstoragelen+=len;
	g_ringbufinfo.canprocesslen-=len;
}
static int isprocessMessage()
{
	int i=0;
	int retval=0;
	int syn_flash=0;
	int packet_flash=0;
	int packetsize=0;
	int detectsize=g_ringbufinfo.canprocesslen;
	int startaddr =g_ringbufinfo.processaddr;
	if(detectsize<3)
		return 0;
	for(i=0;i<detectsize;i++)
	{
		int ind=i+startaddr;
		g_ringbufinfo.canprocesslen-=1;
		g_ringbufinfo.canstoragelen+=1;
		if(ind>BUFSIZE-1)
			ind-=BUFSIZE;

		if(ringbuf[ind]==SYN_SIGN){
			syn_flash=1;
			continue;
		}
		if(syn_flash){
			switch(ringbuf[ind])
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
			packetsize=ringbuf[ind];
			if(g_ringbufinfo.canprocesslen<packetsize-2){				
				g_ringbufinfo.canprocesslen+=2;
				g_ringbufinfo.canstoragelen-=2;
				g_ringbufinfo.processaddr=ind-2;
				retval=0;
			}
			else{
				g_ringbufinfo.canprocesslen+=1;
				g_ringbufinfo.canstoragelen-=1;
				g_ringbufinfo.processaddr=ind-1;				
				retval=1;
			}
			break;
		}
		g_ringbufinfo.processaddr=ind;			
	}
	return retval;
}

static void *uartRead(void * threadParameter)
{
	char *rx;
	int iores, iocount=0;
	printf("uartRead thread \n");
	while(g_rrun) { 
		iores = ioctl(g_fd_uart, FIONREAD, &iocount);
		if(iocount){
         	rx = malloc(iocount);
        	iores = read(g_fd_uart, rx, iocount);
        	storagetoRingbuf(rx,iocount);
        	free(rx);
        	if(isprocessMessage()){
        		rx=malloc(6);
        		getfromRingbuf(rx,6);
        		for(iocount=0;iocount<6;iocount++)
        		printf("0x%x ",rx[iocount]);
        		printf("\n");
        		printf("%d %d %d %d\n",g_ringbufinfo.processaddr,g_ringbufinfo.canprocesslen,g_ringbufinfo.storageaddr,g_ringbufinfo.canstoragelen);
        		//message_resolver(rx);
        		free(rx);
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
		g_isTxfinished=TRUE;
		write(g_fd_uart,g_txbuf,g_txlen);
		g_isTxfinished=FALSE;
	}
	pthread_exit(NULL);
}

int uartInit(int argc ,char **argv)
{
	struct termios options;
	unsigned long baudrate = DEFAULT_RATE;
	int ret,i;

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
	//options.c_cflag |= CRTSCTS;

	options.c_lflag &= ~(ICANON | IEXTEN | ISIG | ECHO| ECHONL);
	options.c_oflag &= ~OPOST;
	options.c_iflag &= ~(ICRNL | INPCK | ISTRIP | IXON | BRKINT );

	options.c_cc[VMIN] = 1;
	options.c_cc[VTIME] = 0;
	options.c_cflag |= (CLOCAL | CREAD);

	for(i = 2; i < argc; i++) {
		
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

	ringbuf=(char *)malloc(BUFSIZE);

	g_ringbufinfo.storageaddr=0;
	g_ringbufinfo.processaddr=0;
	g_ringbufinfo.canstoragelen=BUFSIZE;
	g_ringbufinfo.canprocesslen=0;

	

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
	
	free(ringbuf);
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

