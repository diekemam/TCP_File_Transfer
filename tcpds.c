/* 
 * tcpds.c, TCP Daemon on server side using UDP functions
 * Michael Diekema, Adam Zink
 */

#include "libs.h"
#include "packet.h"

/* tcp daemon called with host name and port number of server */
int main (int argc, char *argv[]) 
{
    if (argc > 1)
	{
	  printf("Usage -- tcpds\n");
	  exit(1);
	}

	int datagram_len = sizeof(struct sockaddr_in);

	int ftps_sock, tcpds_sock, tcpdc_sock;
	struct sockaddr_in datagram, ftps_datagram, tcpdc_datagram;
	Troll_Packet trollPacket;
	TCP_Packet tcpdsPacket;
	
	struct hostent *hp, *lp, *gethostbyname();

	/* create socket for receiving from troll and ftps*/
	tcpds_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (tcpds_sock < 0) 
	{
	    perror("error opening tcpds_sock\n");
		exit(2);
	}

	/* create socket for sending to ftps */
	ftps_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (ftps_sock < 0) 
	{
		perror("error opening ftps_sock\n");
		exit(2);
	}

	/* create socket for forwarding acks to tcpdc */
	tcpdc_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (tcpdc_sock < 0)
	{
		perror("error opening tcpdc_dock\n");
		exit(2);
	}

	/* receive incoming information in datagram, which listens for any sender */
	datagram.sin_family = AF_INET;
	datagram.sin_port = htons(TCPDS_PORT);
	datagram.sin_addr.s_addr = INADDR_ANY;

	/* Bind tcpds_sock so it is listening on TCPDS_PORT for any sender */
	if (bind(tcpds_sock, (struct sockaddr *)&datagram, sizeof(datagram)) < 0)
	{
	    perror("error binding tcpds_sock\n");
		exit(3);
	}

	/* construct name for sending to ftps */
	ftps_datagram.sin_family = AF_INET;
	ftps_datagram.sin_port = htons(FTPS_PORT);

	/* convert troll hostname to IP address and enter into name */
    hp = gethostbyname(SRV_HOST_NAME);
	if (hp == 0) 
	{
	    fprintf(stderr, "%s:unknown host\n", SRV_HOST_NAME);
		exit(3);
	}
    bcopy((char *)hp->h_addr, (char *)&ftps_datagram.sin_addr, hp->h_length);

	/* construct name for sending to tcpdc */
	tcpdc_datagram.sin_family = AF_INET;
	tcpdc_datagram.sin_port = htons(TCPDC_PORT);

	/* convert troll hostname to IP address and enter into name */
    lp = gethostbyname(CLI_HOST_NAME);
	if (lp == 0) 
	{
	    fprintf(stderr, "%s:unknown host\n", CLI_HOST_NAME);
		exit(3);
	}
    bcopy((char *)lp->h_addr, (char *)&tcpdc_datagram.sin_addr, lp->h_length);

	printf("TCPDS listening on port %d\n", ntohs(datagram.sin_port));
	printf("TCPDS sending to FTPS on port %d\n", ntohs(ftps_datagram.sin_port));
	printf("TCPDS forward acks to TCPDC on port %d\n", ntohs(tcpdc_datagram.sin_port));

	int bytes_recv = MAX_BUF_SIZE;
	int bytes_sent = 0;

    /* Make sure no FIN, SYN, etc. flags are set when sending the file data over to ftps */
	trollPacket.packet.flags = 0;

	/* Receive image data until the FIN bit has been set */
    while ( (trollPacket.packet.flags & 0x1) == 0) 
	{
		/* receives packets from troll process */
		bytes_recv = recvfrom(tcpds_sock, (char *)&trollPacket, sizeof(trollPacket), 0, (struct sockaddr *)&datagram, &datagram_len);

		if (bytes_recv < 0) 
		{
			perror("error reading on socket:tcpds\n");
			exit(6);
		}
		
		/* compute the checksum and compare to packet header value */
		uint16_t crc = checksum(trollPacket.packet.data);
		printf("server checksum = %04x, packet checksum = %04x\n", crc, trollPacket.packet.checksum);
		
		if (crc == trollPacket.packet.checksum)
		{
			/*forward buffer message from daemon to ftps*/
			bytes_sent = sendto(ftps_sock, (char *)&trollPacket.packet, sizeof(trollPacket.packet), 0, (struct sockaddr *)&ftps_datagram, sizeof(ftps_datagram));

			if (bytes_sent < 0) 
			{
				perror("error sending on socket\n");
				exit(6);
			}
			printf("Forwarding sequence num %u to ftps\n", trollPacket.packet.seqNum);

			/* If we receive a FIN packet, forward it to ftps and close the connection */
			if ( (trollPacket.packet.flags & 0x1) == 1)
				break;

			bytes_recv = recvfrom(tcpds_sock, (char *)&tcpdsPacket, sizeof(tcpdsPacket), 0, (struct sockaddr *)&datagram, &datagram_len);

			if (bytes_recv < 0)
			{
			  perror("error receiving ack on socket\n");
			  exit(6);
			}
			
			printf("Forwarding ackNum %u to tcpdc\n", tcpdsPacket.ackNum);

			bytes_sent = sendto(tcpdc_sock, (char *)&tcpdsPacket, sizeof(tcpdsPacket), 0, (struct sockaddr *)&tcpdc_datagram, sizeof(tcpdc_datagram)); 

			if (bytes_sent < 0)
			{
				perror("error sending ack to tcpdc\n");
				exit(6);
			}	
		}
		else
		{
			/* invalid checksum, dropping packet */
			printf("Packet dropped\n");
		}
	}

	printf("Finished forwarding file to ftps\n");
	return 0;
}
