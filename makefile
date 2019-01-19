pi2cServer.0: pi2cServer.c
	gcc -g -Wall -c pi2cServer.c -o pi2cServer.o

pi2cServer: pi2cServer.o
	gcc -o pi2cServer pi2cServer.o -l bcm2835
