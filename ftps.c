/* 
 * ftps.c, File Transfer Server using UDP
 * Modified from server.c, UDP version
 * Michael Diekema, Adam Zink
 * 
 * Run ftps on kappa.cse.ohio-state.edu
 */

#include "libs.h"
#include "file_transfer.h"
#include "packet.h"
#include "tcpd_functions.h"

int datagram_len, tcpds_sock, ftps_sock;
struct sockaddr_in ftps_datagram, tcpds_datagram;
struct hostent *serverAddress, *gethostbyname();
TCP_Packet ftpsPacket;
FILE *fp;

void initializeData()
{
    datagram_len  = sizeof(struct sockaddr_in);

    /* Open UDP sockets for receiving seqNums and sending acks*/
    tcpds_sock = tcpd_socket(AF_INET, SOCK_DGRAM, 0);
	ftps_sock = tcpd_socket(AF_INET, SOCK_DGRAM, 0);

    /* Construct datagram for receiving from tcpds */
    ftps_datagram.sin_family = AF_INET;
    ftps_datagram.sin_port = htons(FTPS_PORT);
    ftps_datagram.sin_addr.s_addr = INADDR_ANY;

	/* construct name for sending acks to tcpds */
	tcpds_datagram.sin_family = AF_INET;
	tcpds_datagram.sin_port = htons(TCPDS_FROM_FTPS_PORT);

	/* convert troll hostname to IP address and enter into name */
    serverAddress = gethostbyname(SRV_HOST_NAME);
	if (serverAddress == 0)
	{
	    fprintf(stderr, "%s:unknown host\n", SRV_HOST_NAME);
		exit(3);
	}
    bcopy((char *)serverAddress->h_addr, (char *)&tcpds_datagram.sin_addr, serverAddress->h_length);

	/* Display port numbers being used */
    printf("Server waiting on port %d\n", ntohs(ftps_datagram.sin_port));
	printf("Sending acks to tcpds on port %d\n", ntohs(tcpds_datagram.sin_port));

	/* Bind ftps_sock so it is listening on FTPS_PORT from any sender */
	tcpd_bind(ftps_sock, &ftps_datagram, datagram_len);
}

void establishConnection()
{
	/* Wait indefinitely until we receive a SYN packet from tcpds */
	tcpd_recvfrom(ftps_sock, (char *)&ftpsPacket, sizeof(ftpsPacket), 0, &ftps_datagram, &datagram_len);

	printf("Received SYN packet from troll, sending ACK to tcpds\n");

	tcpd_sendto(tcpds_sock, (char *)&ftpsPacket, sizeof(ftpsPacket), 0, &tcpds_datagram, sizeof(tcpds_datagram));
}

void openFile(char *fileName, char *mode)
{
	/* Open file to write */
	fp = fopen(fileName, mode);
	if (!fp) 
	{
		perror("error opening file to transfer\n");
		exit(5);
	}
}

/*
 *The file transfer server takes no arguments - it
 *simply waits to receive data from the sender on a
 *predefined port.  It opens a UDP socket, opens a new
 *file to write, creates a message to accept incoming
 *data messages from the client, and receives data 
 *until getting an empty packet, indicating the end
 *of the file.
 */
int main (int argc, char *argv[])
{
    if (argc > 1)
	{
        printf("Usage -- ftps\n");
        exit(1);
	}

	/* Crate sockets, create packets, etc. */
	initializeData();
	
	/* Receive SYN packet from tcpds and forward ACK to ftpc */
	establishConnection();

	/* Open file MyImage1.jpg for writing */
	openFile("MyImage1.jpg", "wb");
	
	int bytes_recv = MAX_BUF_SIZE, bytes_sent = MAX_BUF_SIZE;
	int bytes_written = 0;

	/* Write data to the file until the FIN bit is set */
	while (1)
	{
	    /* Read from sock and place in buf */
		bzero(&ftpsPacket, sizeof(ftpsPacket));
		bytes_recv = tcpd_recvfrom(ftps_sock, &ftpsPacket, sizeof(ftpsPacket), 0, &ftps_datagram, &datagram_len);

		/* Stop writing to file once the FIN flag is set */
		if ( (ftpsPacket.flags & 0x1) > 0)
		    break;
		
		/* Write received message to file */
		if (bytes_recv > 0)
		{
		  bytes_written = fwrite(ftpsPacket.data, 1, sizeof(ftpsPacket.data), fp);
		}
		
		ftpsPacket.ackNum = ftpsPacket.seqNum + 1;
		printf("Received sequence number %u, sending acknowledgement number %u\n", ftpsPacket.seqNum, (ftpsPacket.ackNum));

		/* Send the acknowledgement back to tcpds */
		bytes_sent = tcpd_sendto(tcpds_sock, &ftpsPacket, sizeof(ftpsPacket), 0, &tcpds_datagram, sizeof(tcpds_datagram));
	}

	printf("Finished writing MyImage1.jpg\n");
	fclose(fp);

    /* Server terminates connection, closes sockets, and exits */
    close(ftps_sock);
	close(tcpds_sock);
    exit(0);
	return 0;
}
