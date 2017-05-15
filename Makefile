CC=arm-linux-gnueabihf-gcc
#CC=arm-fsl-linux-gnueabi-gcc
uarttest.out:uart.c crc8.c message.c test.c capturetimer.c ringbuffer.c 
	${CC} $^ -lpthread -o $@

clean:
	rm uarttest.out	
