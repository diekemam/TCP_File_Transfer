# Makefile for file transfer project

CC = gcc
OBJCHK = checksum.c
OBJCLI = ftpc.c
OBJSRV = ftps.c
OBJTCPDC = tcpdc.c
OBJTCPDS = tcpds.c
CFLAGS = -Wall -ansi -lxnet

# setup for system
LIBS =

all: checksum.o ftpc ftps tcpdc.o tcpdc tcpds.o tcpds

checksum.o:	$(OBJCHK)
	$(CC) $(CFLAGS) -c $(OBJCHK)

ftpc:	$(OBJCLI)
	$(CC) $(CFLAGS) -o $@ $(OBJCLI) $(LIBS)

ftps:	$(OBJSRV)
	$(CC) $(CFLAGS) -o $@ $(OBJSRV) $(LIBS)

tcpdc.o: $(OBJTCPDC)
	$(CC) $(CFLAGS) -c $(OBJTCPDC)

tcpdc:	tcpdc.o checksum.o
	$(CC) $(CFLAGS) -o $@ tcpdc.o checksum.o

tcpds.o: $(OBJTCPDS)
	$(CC) $(CFLAGS) -c $(OBJTCPDS)

tcpds:  tcpds.o checksum.o
	$(CC) $(CFLAGS) -o $@ tcpds.o checksum.o

clean:
	rm checksum.o ftpc ftps tcpdc.o tcpdc tcpds.o tcpds
