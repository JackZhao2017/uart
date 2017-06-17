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
#define TXBUFSIZE 256
int  g_isTxfinished=FALSE;

RINGBUFFER g_ringbufInfo;

static void *uartRead(void * threadParameter)
{
	u8 data[RINGBUFSIZE];
	int iores, iocount=0,len=0,isSync=0,retval=0;
	int i =0;
	struct timeval tv;
	fd_set r_fds;
	printf("uartRead thread \n");
	while(g_rrun) { 
		FD_ZERO(&r_fds);
		FD_SET(g_fd_uart,&r_fds);
		tv.tv_sec = 1;
		tv.tv_usec = 0;	
		iores = select(g_fd_uart + 1,&r_fds,NULL,NULL,&tv);
		switch(iores){
			case -1:
				printf("-----------------------------select\n");
 				break;
 			case 0:
  				printf("-----------------------------time out\n");
				break;
			default:
				iores = ioctl(g_fd_uart, FIONREAD, &iocount);
				if(iocount){
						memset(data,0,sizeof(data));
        				iores = read(g_fd_uart, data, iocount);
        				putdatatoBuffer(&g_ringbufInfo,data,iocount);
				}
				break;
		}

		if(!isSync)
        		isSync=detectSync(&g_ringbufInfo,SYN_SIGN);
        if(isSync){
        	if(detectMsginfo(&g_ringbufInfo,&len)){
        		if(len){
        			getdatafromBuffer(&g_ringbufInfo,data,len);
        			printf("\n--------------------len  %d\n",len);
        			for(i=0;i<len;i++)
        		   		printf("0x%x ",data[i]);
        		   	printf("\n---------------------------\n");
        			retval=message_resolver(data);
        			if(retval<0)
        				printf("%s resolver faild \n",__func__);
        			isSync=0;
        		}
        	}else{
        			isSync=0;
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
	memset(g_txbuf,0,TXBUFSIZE);
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
	g_txbuf=malloc(TXBUFSIZE);
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
	if(g_txbuf)
		free(g_txbuf);
	printf("%s\n",__func__ );
}

