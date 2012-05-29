/* 
 * ftpc.c, File Transfer Client using UDP
 * Modified from client.c, UDP version
 * Michael Diekema, Adam Zink, 4/24/12
 * 
 * Run ftpc on mu.cse.ohio-state.edu
*/

#include "libs.h"
#include "packet.h"
#include "tcpd_functions.h"
#include "file_transfer.h"
#include "tcpd_buf.h"

int datagram_len, tcpdc_sock, ftpc_sock;
struct sockaddr_in ftpc_datagram, tcpdc_datagram;
struct hostent *lp, *gethostbyname();
struct timeval timeout;
fd_set fds;
TCP_Packet ftpcPacket;
FILE *fp;

void initializeData()
{
	datagram_len = sizeof(struct sockaddr_in);

	/* create sockets for connecting to server and receiving from tcpdc */
	tcpdc_sock = tcpd_socket(AF_INET, SOCK_DGRAM, 0);
	ftpc_sock = tcpd_socket(AF_INET, SOCK_DGRAM, 0);

	/* construct datagram for receiving from tcpdc */
	ftpc_datagram.sin_family = AF_INET;
	ftpc_datagram.sin_port = htons(FTPC_PORT);
	ftpc_datagram.sin_addr.s_addr = INADDR_ANY;

	/* construct tcpdc_datagram for sending packets to tcpdc */
	tcpdc_datagram.sin_family = AF_INET;
	tcpdc_datagram.sin_port = htons(TCPDC_FROM_FTPC_PORT);

	/* convert tcpdc hostname to IP address and enter into tcpdc_datagram */
	lp = gethostbyname(CLI_HOST_NAME);
	if (lp == 0) 
	{
		fprintf(stderr, "%s:unknown host\n", CLI_HOST_NAME);
		exit(3);
	}
	bcopy((char *)lp->h_addr, (char *)&tcpdc_datagram.sin_addr, lp->h_length);

	/* Bind ftpc_sock so it is listening on FTPC_PORT for any sender */
	tcpd_bind(ftpc_sock, &ftpc_datagram, datagram_len);

	/* Set timeout value to .2 seconds */
	timeout.tv_sec = 0;
	timeout.tv_usec = 200000;
}

void openFile(char *filename, char *mode)
{
	fp = fopen(filename, mode);
	if (!fp) 
    {
		perror("error opening file to transfer\n");
		exit(4);
	}
}

void establishConnection()
{
    int result = 0;

	/* Turn on the SYN bit when setting up the connection */
	ftpcPacket.flags = 0x2;
	do
	{
	    tcpd_sendto(tcpdc_sock, &ftpcPacket, sizeof(ftpcPacket), 0, &tcpdc_datagram, datagram_len);
	    printf("Sending SYN packet to server\n");
	    FD_ZERO(&fds);
	    FD_SET(ftpc_sock, &fds);
	    /* If ftpc_sock has received an ACK before it times out, start sending the actual file data over to ftps */
	    result = select(FD_SETSIZE,  &fds, NULL, NULL, &timeout);

		/* If ftpc_sock has not received an ACK, resend the SYN packet */
		if (result == 0)
		    continue;

	    tcpd_recvfrom(ftpc_sock, &ftpcPacket, sizeof(ftpcPacket), 0, &ftpc_datagram, &datagram_len);
	} while(result == 0);
}

/*
 *The file transfer client takes a filename to transfer as its only
 *argument.  It opens a UDP socket, creates a message for the server,
 *packs the message within a packet that is sent to the troll, opens
 *the specified file, and sends MTU sized data buffers to the server
 *process.
*/
int main(int argc, char *argv[])
{
    /* Check if the filename is passed to ftpc */
    if (argc < 2) 
    {
        printf("usage: ftpc filename\n");
        exit(1);
    }
	
	/* Create sockets, create packets, etc. */
	initializeData();

	/* First, establish the connection with the server. Keep sending packets with the SYN bit set until an ACK is received */
	establishConnection();

	printf("Connection established...\n");
	
	/* Open file to transfer */
	char *filename = argv[1];
	openFile(filename, "rb");

	printf("Client sending filename: %s\n", filename);
	printf("Sending file...\n");
	
	int bytes_read = 1, bytes_sent = 0, bytes_recv = 0, bytes_total = 0;

	/* Generate an unsigned 32 bit random number using the current time as the seed. Assign the random number to the sequence number field in TCP_Packet */
    uint32_t randSeed = srand48(time(NULL));
    ftpcPacket.seqNum = (uint32_t)mrand48(randSeed);

	/* Send image data to tcpdc until the entire file has been read */
	while (bytes_read > 0) 
    {
		/* sleep ~10 ms to space packets received by troll */
        usleep(10000);

		/* read part of file into buf */
		bzero(ftpcPacket.data, MAX_BUF_SIZE);
		bytes_read = fread(ftpcPacket.data, 1, MAX_BUF_SIZE, fp);
		
		/* Stop writing data once we've reached the end of the file */
		if (bytes_read == 0)
		    break;

		/* Since we are sending file data over to tcpdc, none of the SYN, FIN, etc. flags are set */
		ftpcPacket.flags = 0;

		/* Wait until the circular buffer isn't full and write buf to sock */
		while (is_tcpd_buf_full())
		{
			usleep(10000);
		}

		/* Now we send the packet over to tcpdc, which is running on the same machine, so we have a reliable connection */
		bytes_sent = tcpd_sendto(tcpdc_sock, &ftpcPacket, sizeof(ftpcPacket), 0, &tcpdc_datagram, sizeof(tcpdc_datagram));

		bytes_total += bytes_read;
		printf("Sending seqNum %u\n", ftpcPacket.seqNum);

		int result = 0;
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
			    bytes_sent = tcpd_sendto(tcpdc_sock, &ftpcPacket, sizeof(ftpcPacket), 0, &tcpdc_datagram, sizeof(tcpdc_datagram));
				printf("Timeout: resending seqNum %u\n", ftpcPacket.seqNum);
			}
		} while(result == 0);


		/* Wait for acknowledgement from ftps */
		bytes_recv = tcpd_recvfrom(ftpc_sock, &ftpcPacket, sizeof(ftpcPacket), 0, &ftpc_datagram, &datagram_len);

		printf("Received ackNum %u\n", ftpcPacket.ackNum);
		ftpcPacket.seqNum++;
	}

	printf("Sent %d bytes\n", bytes_total);

	/* Now that we are done sending the file, we send a packet with the FIN bit set to indicate that the connection should stop */
	bzero(ftpcPacket.data, MAX_BUF_SIZE);

    /* Set the FIN flag to indicate that we want to close the connection */
	ftpcPacket.flags = 0x1;

	bytes_sent = tcpd_sendto(tcpdc_sock, &ftpcPacket, sizeof(ftpcPacket), 0, &tcpdc_datagram, sizeof(tcpdc_datagram));

	printf("Finished sending %s\n", filename);
	fclose(fp);

    /* close connection */
    close(tcpdc_sock);
	close(ftpc_sock);
    exit(0);
	return 0;
}
