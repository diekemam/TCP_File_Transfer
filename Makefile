# Makefile for file transfer project

CC = gcc
OBJCHK = checksum.c
OBJCLI = ftpc.c
OBJSRV = ftps.c
OBJTCPDC = tcpdc.c
OBJTCPDS = tcpds.c
OBJTCPDF = tcpd_functions.c
OBJTCPDB = tcpd_buf.c
OBJDLTMR = deltatimer.c
CFLAGS = -Wall -ansi
CFLAGSLINK = -Wall -ansi -lxnet

# setup for system
LIBS = 

all: checksum.o tcpd_functions.o tcpd_buf.o deltatimer.o ftpc.o ftps.o tcpdc.o tcpds.o ftpc ftps tcpdc  tcpds

checksum.o:	$(OBJCHK)
	$(CC) $(CFLAGS) -c $(OBJCHK)

tcpd_functions.o: $(OBJTCPDF)
	$(CC) $(CFLAGS) -c $(OBJTCPDF)

tcpd_buf.o: $(OBJTCPDB)
	$(CC) $(CFLAGS) -c $(OBJTCPDB)

deltatimer.o: $(OBJDLTMR)
	$(CC) $(CFLAGS) -c $(OBJDLTMR)

ftpc.o: $(OBJCLI)
	$(CC) $(CFLAGS) -c $(OBJCLI)

ftps.o: $(OBJSRV)
	$(CC) $(CFLAGS) -c $(OBJSRV)

tcpdc.o: $(OBJTCPDC)
	$(CC) $(CFLAGS) -c $(OBJTCPDC)

tcpds.o: $(OBJTCPDS)
	$(CC) $(CFLAGS) -c $(OBJTCPDS)

ftpc:	ftpc.o tcpd_functions.o tcpd_buf.o
	$(CC) $(CFLAGSLINK) -o $@ ftpc.o tcpd_functions.o tcpd_buf.o

ftps:	ftps.o tcpd_functions.o
	$(CC) $(CFLAGSLINK) -o $@ ftps.o tcpd_functions.o

tcpdc:	tcpdc.o checksum.o tcpd_functions.o
	$(CC) $(CFLAGSLINK) -o $@ tcpdc.o checksum.o tcpd_functions.o

tcpds:  tcpds.o checksum.o tcpd_functions.o
	$(CC) $(CFLAGSLINK) -o $@ tcpds.o checksum.o tcpd_functions.o

clean:
	rm checksum.o tcpd_functions.o tcpd_buf.o deltatimer.o ftpc.o ftps.o tcpdc.o tcpds.o ftpc ftps tcpdc tcpds
