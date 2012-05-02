# Makefile for file transfer project

CC = gcc
OBJCLI = ftpc.c 
OBJSRV = ftps.c
OBJTCPDC = tcpdc.c
OBJTCPDS = tcpds.c
CFLAGS = -Wall -ansi -lxnet

# setup for system
LIBS =

all: ftpc ftps tcpdc tcpds

ftpc:	$(OBJCLI)
	$(CC) $(CFLAGS) -o $@ $(OBJCLI) $(LIBS)

ftps:	$(OBJSRV)
	$(CC) $(CFLAGS) -o $@ $(OBJSRV) $(LIBS)

tcpdc:	$(OBJTCPDC)
	$(CC) $(CFLAGS) -o $@ $(OBJTCPDC) $(LIBS)

tcpds:  $(OBJTCPDS)
	$(CC) $(CFLAGS) -o $@ $(OBJTCPDS) $(LIBS)

clean:
	rm ftpc ftps tcpdc tcpds
