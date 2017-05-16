#ifndef _CMDQUEUE_H_
#define _CMDQUEUE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "message.h"

int putcmdintoQueue(SYS_CTRLINFO ctrlinfo);
int getcmdfromQueue(SYS_CTRLINFO *ctrlinfo);	
int iscmdneedProcess(void);

#ifdef __cplusplus
};
#endif

#endif