/* 
 * ftpc.c, File Transfer Client using UDP
 * Modified from client.c, UDP version
 * Adam Zink, 4/24/12
 * 
 * Run ftpc on mu.cse.ohio-state.edu
 */

#include "libs.h"
#include "packet.h"

/*
 * The file transfer client takes a filename to transfer as its only argument.  It opens
 * a UDP socket, creates a message for the server, packs the message within a packet
 * that is sent to the troll, opens the specified file, and sends MTU sized data buffers
 * to the server process.
 */
int main(int argc, char *argv[])
{
    /* Check if the filename is passed to ftpc */
    if (argc < 2) 
    {
        printf("usage: ftpc filename\n");
        exit(1);
    }

	int datagram_len = sizeof(struct sockaddr_in);
	int tcpdc_sock, ftpc_sock;
    struct sockaddr_in datagram, tcpdc_datagram;
    struct hostent *lp, *gethostbyname();
	TCP_Packet ftpcPacket; /* create a TCP packet to send over the network */

    /* Generate an unsigned 32 bit random number using the current time as the seed. Assign the random number to the sequence number field in TCP_Packet */
    uint32_t randSeed = srand48(time(NULL));
    ftpcPacket.seqNum = (uint32_t)mrand48(randSeed);

	char *filename = argv[1];

    /* create socket for connecting to server */
    tcpdc_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (tcpdc_sock < 0) 
    {
		perror("opening datagram socket");
		exit(2);
    }

	/* create socket for receiving from tcpdc */
    ftpc_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (ftpc_sock < 0) 
    {
		perror("opening datagram socket");
		exit(2);
    }

    /* construct tcpdc_datagram for sending packets to tcpdc */
    tcpdc_datagram.sin_family = AF_INET;
    tcpdc_datagram.sin_port = htons(TCPDC_PORT);

    /* convert tcpdc hostname to IP address and enter into tcpdc_datagram */
    lp = gethostbyname(CLI_HOST_NAME);
    if (lp == 0) 
    {
		fprintf(stderr, "%s:unknown host\n", CLI_HOST_NAME);
		exit(3);
    }
    bcopy((char *)lp->h_addr, (char *)&tcpdc_datagram.sin_addr, lp->h_length);

	/* construct datagram for receiving from tcpdc */
	datagram.sin_family = AF_INET;
	datagram.sin_port = htons(FTPC_PORT);
	datagram.sin_addr.s_addr = INADDR_ANY;

	/* Bind ftpc_sock so it is listening on FTPC_PORT for any sender */
	if (bind(ftpc_sock, (struct sockaddr *)&datagram, sizeof(datagram)) < 0)
	{
		perror("error binding datagram socket\n");
		exit(2);
	}
	
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
	
	int bytes_read = 1, bytes_sent = 0, bytes_recv = 0, bytes_total = 0;

	/* The delta timer will cause ftpc to resend a packet if it times out */
	int result = 0;
	fd_set fds;
	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 100000;

	/* Send image data to tcpdc until the entire file has been read */
	while (bytes_read > 0) 
    {
		/* sleep ~10 ms to space packets received by troll */
		int i;
		for (i=0; i<500000; i++);
		
		/* read part of file into buf */
		bzero(ftpcPacket.data, MAX_BUF_SIZE);
		bytes_read = fread(ftpcPacket.data, 1, MAX_BUF_SIZE, fp);
		
		/* Stop writing data once we've reached the end of the file */
		if (bytes_read == 0)
		    break;

		/* Since we are sending file data over to tcpdc, none of the SYN, FIN, etc. flags are set */
		ftpcPacket.flags = 0;

		/* write buf to sock */
		bytes_sent = sendto(tcpdc_sock, &ftpcPacket, sizeof(ftpcPacket), 0, (struct sockaddr *)&tcpdc_datagram, sizeof(tcpdc_datagram));

		if(bytes_sent < 0) 
        {
			perror("error writing on socket");
			exit(6);
		}

		bytes_total += bytes_read;
		printf("Sending seqNum %u\n", ftpcPacket.seqNum);

		/* At this point, ftpc will either receive an ackNum or timeout if the packet was dropped */
		do
		{
		    FD_ZERO(&fds);
		    FD_SET(ftpc_sock, &fds);
		    result = select(sizeof(fds) * 8, &fds, NULL, NULL, &timeout);

			/* If ftpc_sock has not received any data before it times out, resend the packet */
		    if (result == 0)
			{
			    /* write buf to sock */
			    bytes_sent = sendto(tcpdc_sock, &ftpcPacket, sizeof(ftpcPacket), 0, (struct sockaddr *)&tcpdc_datagram, sizeof(tcpdc_datagram));

				printf("Timeout: resending seqNum %u\n", ftpcPacket.seqNum);
				if(bytes_sent < 0) 
				{
					perror("error writing on socket");
					exit(6);
				}
			}
		} while(result == 0);


		/* Wait for acknowledgement from ftps */
		bytes_recv = recvfrom(ftpc_sock, &ftpcPacket, sizeof(ftpcPacket), 0, (struct sockaddr *)&datagram, &datagram_len);
		printf("Received ackNum %u\n", ftpcPacket.ackNum);

		ftpcPacket.seqNum++;
	}

	printf("Sent %d bytes\n", bytes_total);

	/* Now that we are done sending the file, we send a packet with the FIN bit set to indicate that the connection should stop */
	bzero(ftpcPacket.data, MAX_BUF_SIZE);

    /* Set the FIN flag to indicate that we want to close the connection */
	ftpcPacket.flags = 0x1;

	bytes_sent = sendto(tcpdc_sock, &ftpcPacket, sizeof(ftpcPacket), 0, (struct sockaddr *)&tcpdc_datagram, sizeof(tcpdc_datagram));

	printf("Finished sending %s\n", filename);
	fclose(fp);

    /* close connection */
    close(socket);
    exit(0);
	return 0;
}
