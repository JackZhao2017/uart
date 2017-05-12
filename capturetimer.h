#ifndef _CAPTURETIMER_H_
#define _CAPTURETIMER_H_
#include <sys/time.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
int capture_timeinit(int mfps);
int capture_timerattrch(void *func);
void wait_timersignal(void);
int capture_timerelease();

#endif




