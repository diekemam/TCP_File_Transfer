/* 
 * ftps.c, File Transfer Server using UDP
 * Modified from server.c, UDP version
 * Michael Diekema, Adam Zink
 * 
 * Run ftps on kappa.cse.ohio-state.edu
 */

#include "libs.h"
#include "packet.h"

/*
 *The file transfer server takes no arguments - it
 *simply waits to receive data from the sender on a
 *predefined port.  It opens a UDP socket, opens a new
 *file to write, creates a message to accept incoming
 *data messages from the client, and receives data 
 *until getting an empty packet, indicating the end
 *of the file.
 */
int main (void)
{
    int inSock, outSock, datagram_len;
    struct sockaddr_in datagram, tcpds_datagram;
	struct hostent *hp, *gethostbyname();
	TCP_Packet ftpsPacket;

    /* Open UDP sockets for receiving seqNums and sending acks*/
    inSock = socket(AF_INET, SOCK_DGRAM, 0);
    if (inSock < 0) {
		perror("opening datagram socket\n");
		exit(1);
    }

	outSock = socket(AF_INET, SOCK_DGRAM, 0);
    if (outSock < 0) {
		perror("opening datagram socket\n");
		exit(1);
    }

    /* create name with parameters and bind name to socket */
    datagram.sin_family = AF_INET;
    datagram.sin_port = htons(FTPS_PORT);
    datagram.sin_addr.s_addr = INADDR_ANY;
    if(bind(inSock, (struct sockaddr *)&datagram, sizeof(datagram)) < 0) 
	{
		perror("getting socket name\n");
		exit(2);
    }

    datagram_len = sizeof(struct sockaddr_in);

	/* construct name for sending to tcpds */
	tcpds_datagram.sin_family = AF_INET;
	tcpds_datagram.sin_port = htons(TCPDS_PORT);

	/* convert troll hostname to IP address and enter into name */
    hp = gethostbyname(SRV_HOST_NAME);
	if (hp == 0) 
	{
	    fprintf(stderr, "%s:unknown host\n", SRV_HOST_NAME);
		exit(3);
	}
    bcopy((char *)hp->h_addr, (char *)&tcpds_datagram.sin_addr, hp->h_length);
	
    /* Display port numbers being used */
    printf("Server waiting on port # %d\n", ntohs(datagram.sin_port));
	printf("Sending acks to tcpds on port %d\n", ntohs(tcpds_datagram.sin_port));

	/* open file to write */
	FILE *fp;
	fp = fopen("MyImage1.jpg", "wb");
	if (!fp) 
	{
		perror("error opening file to transfer\n");
		exit(5);
	}
	
	int bytes_recv = MAX_BUF_SIZE, bytes_sent = MAX_BUF_SIZE;
	int bytes_written = 0;

	/* Write data to the file until the FIN bit is set */
	while ( (ftpsPacket.flags & 01) == 0)
	{
		/* read from sock and place in buf */
		bzero(&ftpsPacket, sizeof(ftpsPacket));
		bytes_recv = recvfrom(inSock, &ftpsPacket, sizeof(ftpsPacket), 0, (struct sockaddr *)&datagram, &datagram_len);

		if(bytes_recv < 0) 
		{
			perror("error reading on socket\n");
			exit(6);
		}

		/* Stop writing to file once the FIN flag is set */
		if ( (ftpsPacket.flags & 01) > 0)
		    break;
		
		/* write received message to file */
		if (bytes_recv > 0) 
		{
			bytes_written = fwrite(ftpsPacket.data, 1, sizeof(ftpsPacket.data), fp);
		}

		ftpsPacket.ackNum = ftpsPacket.seqNum + 1;
		printf("Received sequence number %u, sending acknowledgement number %u\n", ftpsPacket.seqNum, (ftpsPacket.seqNum) + 1);

		/* Send the acknowledgement back to tcpds */
		bytes_sent = sendto(outSock, &ftpsPacket, sizeof(ftpsPacket), 0, (struct sockaddr *)&tcpds_datagram, datagram_len);

		if (bytes_sent < 0)
		{
			perror("error writing to socket\n");
			exit(6);
		}
	}

	printf("Finished writing MyImage1.jpg\n");
	fclose(fp);

    /* Server terminates connection, closes sockets, and exits */
    close(inSock);
	close(outSock);
    exit(0);
	return 0;
}
