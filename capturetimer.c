#include "capturetimer.h"
#include <pthread.h>
#include <semaphore.h>

pthread_t g_threadc;
sem_t g_semcapture;
struct itimerval g_timevalue;          //(1)

#define FALSE 0
#define TRUE 1
int exitThread=FALSE;
int g_finished=TRUE;
int g_iscreat=FALSE;
static long long currenttime(){
    struct timeval now;
    gettimeofday(&now, NULL);
    long long when = now.tv_sec * 1000LL + now.tv_usec / 1000;
    return when;
}
void *thread_capture(void *func)
{
	int (*signal_process)();
	signal_process=func;
	printf("<%s>:create capture thread  \n",__func__);
	while(1){
		sem_wait(&g_semcapture);
		if(exitThread){
			break;
		}
		g_finished=FALSE;		
		signal_process();	
		g_finished=TRUE;
	}
	printf("<%s>:Exit capture time  thread\n",__func__);
	pthread_exit(NULL);
}

void wait_timersignal(void)
{
	g_finished=TRUE;
	sem_wait(&g_semcapture);
	g_finished=FALSE;
}

static void capture_signal(int signo)
{	
	switch(signo){
		case SIGALRM:		
			if(g_finished){	
				sem_post(&g_semcapture);
			}
			break;
	}	
}
int capture_timeinit(int mfps)
{
    if(mfps<1)
		return -1;
	memset(&g_timevalue,0,sizeof(g_timevalue));
    printf("<%s>:process id is %d\n",__func__, getpid());	
	sem_init(&g_semcapture,0,0);	
    signal(SIGALRM, capture_signal);
    g_timevalue.it_value.tv_sec = 0;
    g_timevalue.it_value.tv_usec = 1000000/mfps;
    g_timevalue.it_interval.tv_sec = 0;
    g_timevalue.it_interval.tv_usec = 1000000/mfps;
    setitimer(ITIMER_REAL, &g_timevalue, NULL);     //(2)
    printf("<%s>:time interrupt frame :%d \n",__func__,g_timevalue.it_value.tv_usec);
	return 0;
}
int capture_timerattrch(void *func)
{
	exitThread=FALSE;
	g_iscreat=TRUE;
	if(pthread_create(&g_threadc,NULL,thread_capture,func)<0){
		return -1;
	}
	return 0;
}
int capture_timerelease()
{
	int ret=0;
	exitThread=TRUE;
	if(g_iscreat){
		ret=pthread_join(g_threadc,NULL);
		if(ret<0)
			printf("<%s>:fail to stop capture timer thread",__func__);
	}
	g_finished=FALSE;
	sem_destroy(&g_semcapture);
	memset(&g_timevalue,0,sizeof(g_timevalue));
	setitimer(ITIMER_REAL, &g_timevalue, NULL);
	signal(SIGALRM, NULL);
	printf("<%s>:capture_timerelease success \n",__func__);	
	return 0;
}

	










