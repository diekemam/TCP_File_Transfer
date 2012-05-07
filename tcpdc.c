/* 
 * tcpdc.c, TCP Daemon on client side using UDP functions
 * Michael Diekema, Adam Zink
 */

#include "libs.h"
#include "packet.h"

/* tcp daemon called with host name and port number of server */
int main (int argc, char *argv[]) 
{
    if (argc > 1)
	{
	  printf("Usage -- tcpdc\n");
	  exit(1);
	}

    int datagram_len = sizeof(struct sockaddr_in);

	int ftpc_sock, tcpdc_sock, troll_sock;
	struct sockaddr_in datagram, tcpds_datagram, ftpc_datagram;
	Troll_Packet trollPacket;
	TCP_Packet tcpdcPacket;
	
	struct hostent *hp, *lp, *gethostbyname();

    /* create socket for receiving from ftpc and tcpds */
	tcpdc_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (tcpdc_sock < 0) 
	{
		perror("opening datagram socket\n");
		exit(2);
	}
			
	/* create socket for sending to troll */
	troll_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (troll_sock < 0) 
	{
		perror("opening datagram socket\n");
		exit(2);
	}

	/* create socket for sending to ftpc */
	ftpc_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (ftpc_sock < 0) 
	{
		perror("opening datagram socket\n");
		exit(2);
	}

	
	/* construct datagram for receiving from ftpc */
	datagram.sin_family = AF_INET;
	datagram.sin_port = htons(TCPDC_PORT);
	datagram.sin_addr.s_addr = INADDR_ANY;

	/* Bind tcpdc_sock so it is listening on TCPDC_PORT for any sender */
	if (bind(tcpdc_sock, (struct sockaddr *)&datagram, sizeof(datagram)) < 0)
	{
		perror("error binding datagram socket\n");
		exit(2);
	}
	printf("TCPDC waiting on port num %d\n", ntohs(datagram.sin_port));

	/* construct datagram for connecting to TCPDS */
	tcpds_datagram.sin_family = AF_INET;
	tcpds_datagram.sin_port = htons(TCPDS_PORT);

	/* convert troll hostname to IP address and enter into name */
	lp = gethostbyname(CLI_HOST_NAME);
	if (lp == 0) 
	{
		fprintf(stderr, "%s:unknown host\n", CLI_HOST_NAME);
		exit(3);
	}
	bcopy((char *)lp->h_addr, (char *)&trollPacket.header.sin_addr, lp->h_length);

	/* construct datagram for forwarding acks to ftpc */
	ftpc_datagram.sin_family = AF_INET;
	ftpc_datagram.sin_port = htons(FTPC_PORT);

	/* convert ftpc hostname to IP address and enter into ftpc_datagram */
	lp = gethostbyname(CLI_HOST_NAME);
	if (lp == 0) 
	{
		fprintf(stderr, "%s:unknown host\n", CLI_HOST_NAME);
		exit(3);
	}
	bcopy((char *)lp->h_addr, (char *)&ftpc_datagram.sin_addr, lp->h_length);
			
	/* prepare message for troll */
	trollPacket.header.sin_family = htons(AF_INET);
	trollPacket.header.sin_port = htons(TROLL_PORT);
			
	/* convert server hostname to IP address and enter into tcpdc_datagram */
	hp = gethostbyname(SRV_HOST_NAME);
	if (hp == 0)
    {
		fprintf(stderr, "%s:unknown host\n", SRV_HOST_NAME);
		exit(3);
	}
	bcopy((char *)hp->h_addr, (char *)&tcpds_datagram.sin_addr, hp->h_length);
	printf("Sending to troll at port %d\n", ntohs(trollPacket.header.sin_port));
	printf("Troll sending to TCPDS at port %d\n", ntohs(tcpds_datagram.sin_port));

	int bytes_sent = 0, bytes_recv = 0;

	/* Make sure no FIN, SYN, etc. flags are set when sending the file data over to tcpds */
	trollPacket.packet.flags = 0;

	/* Receive image data until the fin bit has been set */
	while ( (trollPacket.packet.flags & 0x1) == 0) 
	{
	    /* receives packets from local ftpc process */
        bzero(&trollPacket.packet, sizeof(trollPacket.packet));

		bytes_recv = recvfrom(tcpdc_sock, (char *)&trollPacket.packet, sizeof(trollPacket.packet), 0, (struct sockaddr *)&datagram, &datagram_len);

		if(bytes_recv < 0) 
        {
			perror("error reading on socket");
			exit(6);
		}
			  
		/* forward buffer message from daemon to troll process */
	    bytes_sent = sendto(troll_sock, (char *)&trollPacket, sizeof(trollPacket), 0, (struct sockaddr *)&tcpds_datagram, sizeof(tcpds_datagram));
		if(bytes_sent < 0)
        {
			perror("error writing on socket");
			exit(6);
		}
		printf("Forwarding sequence num %u to tcpds through troll\n", trollPacket.packet.seqNum); 

		/* Stop transmitting data once the FIN bit has been set */
		if ( (trollPacket.packet.flags & 0x1) == 1)
		    break;

		bytes_recv = recvfrom(tcpdc_sock, (char *)&tcpdcPacket, sizeof(tcpdcPacket), 0, (struct sockaddr *)&datagram, &datagram_len);

		if (bytes_recv < 0)
		{
		    perror("error reading on socket\n");
		    exit(6);
		}

		bytes_sent = sendto(ftpc_sock, (char *)&tcpdcPacket, sizeof(tcpdcPacket), 0, (struct sockaddr *)&ftpc_datagram, sizeof(ftpc_datagram));

		if (bytes_sent < 0)
		{
		    perror("error sending on socket\n");
		    exit(6);
		}

		printf("Forwarding ackNum %u to ftpc\n", tcpdcPacket.ackNum);
				
	}

	printf("Finished forwarding file\n");
	return(0);
}
