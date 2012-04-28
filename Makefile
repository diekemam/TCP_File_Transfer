# Makefile for ftpc and ftps

CC = gcc
OBJCLI = ftpc.c 
OBJSRV = ftps.c
OBJTCPD = tcpd.c
CFLAGS = -Wall -ansi -lxnet
# setup for system
LIBS =

all: ftpc ftps tcpd

ftpc:	$(OBJCLI)
	$(CC) $(CFLAGS) -o $@ $(OBJCLI) $(LIBS)

ftps:	$(OBJSRV)
	$(CC) $(CFLAGS) -o $@ $(OBJSRV) $(LIBS)
	
tcpd:	$(OBJTCPD)
	$(CC) $(CFLAGS) -o $@ $(OBJTCPD) $(LIBS)

clean:
	rm ftpc ftps tcpd
