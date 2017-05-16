#include "message.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define CMDQUEUENUM 256
static SYS_CTRLINFO   g_sysctrl_rx[256];
int g_putqueueid=0;
int g_outqueueid=0;
static int nextqueueid(int id)
{	
	return (id+1)<CMDQUEUENUM?(id+1):0;
}
int iscmdneedProcess(void)
{
	return g_putqueueid==g_outqueueid?0:1;
}
int putcmdintoQueue(SYS_CTRLINFO ctrlinfo)
{
	memset(&g_sysctrl_rx[g_putqueueid],0,sizeof(SYS_CTRLINFO));
	g_sysctrl_rx[g_putqueueid].Seqnum=ctrlinfo.Seqnum;
	g_sysctrl_rx[g_putqueueid].Commad=ctrlinfo.Commad;
	if(ctrlinfo.datalen){
		g_sysctrl_rx[g_putqueueid].data=malloc(ctrlinfo.datalen);
		g_sysctrl_rx[g_putqueueid].datalen=ctrlinfo.datalen;
		memcpy(g_sysctrl_rx[g_putqueueid].data,ctrlinfo.data,ctrlinfo.datalen);
	}
	g_sysctrl_rx[g_putqueueid].flag=1;
	g_putqueueid=nextqueueid(g_putqueueid);
}
int getcmdfromQueue(SYS_CTRLINFO *ctrlinfo)
{
	ctrlinfo->Seqnum=g_sysctrl_rx[g_outqueueid].Seqnum;
	ctrlinfo->Commad=g_sysctrl_rx[g_outqueueid].Commad;
	if(g_sysctrl_rx[g_outqueueid].datalen){
		ctrlinfo->datalen=g_sysctrl_rx[g_outqueueid].datalen;
		ctrlinfo->data=malloc(ctrlinfo->datalen);
		memcpy(ctrlinfo->data,g_sysctrl_rx[g_outqueueid].data,ctrlinfo->datalen);
		free(g_sysctrl_rx[g_outqueueid].data);
	}
	g_outqueueid=nextqueueid(g_outqueueid);
}
