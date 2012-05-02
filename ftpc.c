/* 
 * ftpc.c, File Transfer Client using UDP
 * Modified from client.c, UDP version
 * Adam Zink, 4/24/12
 * 
 * Run ftpc on mu.cse.ohio-state.edu
 */

#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include "file_transfer.h"

/*
 * The file transfer client takes a filename to transfer as its only argument.  It opens
 * a UDP socket, creates a message for the server, packs the message within a packet
 * that is sent to the troll, opens the specified file, and sends MTU sized data buffers
 * to the server process.
 */
int main(int argc, char *argv[])
{
    if (argc < 2) 
    {
        printf("usage: ftpc filename\n");
        exit(1);
    }

    int tcpd_sock;
    struct sockaddr_in datagram;
    struct hostent *lp, *gethostbyname();
	char buf[MAX_BUF_SIZE];

	char *filename = argv[1];

    /* create socket for connecting to server */
    tcpd_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (tcpd_sock < 0) 
    {
		perror("opening datagram socket");
		exit(2);
    }

    /* construct name for connecting to troll */
    datagram.sin_family = AF_INET;
    datagram.sin_port = htons(TCPDC_PORT);

    /* convert troll hostname to IP address and enter into name */
    lp = gethostbyname(CLI_HOST_NAME);
    if (lp == 0) 
    {
		fprintf(stderr, "%s:unknown host\n", CLI_HOST_NAME);
		exit(3);
    }
    bcopy((char *)lp->h_addr, (char *)&datagram.sin_addr, lp->h_length);
	
	/* open file to transfer */
	FILE *fp;
	fp = fopen(filename, "rb");
	if (!fp) 
    {
		perror("error opening file to transfer");
		exit(4);
	}
	
	printf("Client sending filename: %s\n", filename);
	printf("Sending file...\n");
	
	int bytes_read = 1;
	int bytes_sent = 0;
	int bytes_total = 0;
	while (bytes_read > 0) 
    {
	  
		/* sleep ~10 ms to space packets received by troll */
		int i;
		for (i=0; i<500000; i++);
		
		/* read part of file into buf */
		bzero(buf, MAX_BUF_SIZE);
		bytes_read = fread(buf, 1, MAX_BUF_SIZE, fp);
		
		/* write buf to sock */
		bytes_sent = sendto(tcpd_sock, buf, bytes_read, 0, (struct sockaddr *)&datagram, sizeof(datagram));
		if(bytes_sent < 0) 
        {
			perror("error writing on socket");
			exit(6);
		}
		bytes_total += bytes_read;
		printf("%d bytes read,\ttotal: %d bytes\n", bytes_read, bytes_total);
	}
	printf("Finished sending %s\n", filename);
	fclose(fp);

    /* close connection */
    close(socket);
    exit(0);
	return 0;
}
