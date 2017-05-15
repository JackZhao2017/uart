#ifndef _UART_H_
#define _UART_H_

int  uartInit(int argc ,char **argv);
int  issendBusy(void);
int  readdataComplete(int val);
int  uartsendData(char *buf ,int len);
void uartRelease(void);

#endif