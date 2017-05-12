CC=arm-linux-gnueabihf-gcc
uarttest.out:uart.c crc8.c message.c test.c capturetimer.c
	${CC} $^ -lpthread -o $@

clean:
	rm uarttest.out	
