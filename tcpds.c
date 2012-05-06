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

	int ftps_sock, tcpds_sock;
	struct sockaddr_in datagram, ftps_datagram;
	Troll_Packet trollPacket;
	
	struct hostent *hp, *gethostbyname();

	/* create socket for receiving from troll */
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

	/* construct name for listening in from tcpds */
	datagram.sin_family = AF_INET;
	datagram.sin_port = htons(TCPDS_PORT);
	datagram.sin_addr.s_addr = INADDR_ANY;
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

	printf("TCPDS listening on port %d\n", ntohs(datagram.sin_port));
	printf("TCPDS sending to FTPS on port %d\n", ntohs(ftps_datagram.sin_port));

	int bytes_recv = MAX_BUF_SIZE;
	int bytes_sent = 0;

    /* Make sure no FIN, SYN, etc. flags are set when sending the file data over to ftps */
	trollPacket.packet.flags = 0;

	/* Receive image data until the FIN bit has been set */
    while ( (trollPacket.packet.flags & 0x1) == 0) 
	{
		/* receives packets from troll process */
		bytes_recv = recvfrom(tcpds_sock, (char *)&trollPacket, sizeof(trollPacket), 0, (struct sockaddr *)&datagram, &datagram_len);

		if(bytes_recv < 0) 
		{
			perror("error reading on socket:tcpds\n");
			exit(6);
		}
		   
		/*forward buffer message from daemon to ftps*/
		bytes_sent = sendto(ftps_sock, (char *)&trollPacket.packet, sizeof(trollPacket.packet), 0, (struct sockaddr *)&ftps_datagram, sizeof(ftps_datagram));

		if(bytes_sent < 0) 
		{
			perror("error sending on socket:tcpds\n");
			exit(6);
		}
		printf("Forwarding sequence num %u to ftps\n", trollPacket.packet.seqNum);
	}

	printf("Finished forwarding file to ftps\n");
	return 0;
}
