/* 
 * tcpds.c, TCP Daemon on server side using UDP functions
 * Michael Diekema, Adam Zink
 */

#include "libs.h"
#include "packet.h"
#include "tcpd_functions.h"
#include "checksum.h"

int datagram_len, ftps_sock, tcpds_sock, tcpdc_sock;
struct sockaddr_in datagram, ftps_datagram, tcpdc_datagram;
struct hostent *hp, *lp, *gethostbyname();
struct timeval timeout;
Troll_Packet trollPacket;
TCP_Packet tcpdsPacket;

void initializeData()
{
	datagram_len = sizeof(struct sockaddr_in);

	/* create sockets for receiving datagrams, sending to ftps, and sending to tcpdc */
	tcpds_sock = tcpd_socket(AF_INET, SOCK_DGRAM, 0);
	ftps_sock = tcpd_socket(AF_INET, SOCK_DGRAM, 0);
	tcpdc_sock = tcpd_socket(AF_INET, SOCK_DGRAM, 0);

	/* receive incoming information in datagram, which listens for any sender */
	datagram.sin_family = AF_INET;
	datagram.sin_port = htons(TCPDS_PORT);
	datagram.sin_addr.s_addr = INADDR_ANY;

	/* Bind tcpds_sock so it is listening on TCPDS_PORT for any sender */
	tcpd_bind(tcpds_sock, &datagram, sizeof(datagram));

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

	/* Set timeout to .1 seconds */
	timeout.tv_sec = 0;
	timeout.tv_usec = 100000;
}

void establishConnection()
{
	/* Wait indefinitely until we receive a SYN packet from the troll */
	tcpd_recvfrom(tcpds_sock, (char *)&trollPacket, sizeof(trollPacket), 0, &datagram, &datagram_len);

	printf("Received SYN packet from troll, forwarding to ftps\n");

	tcpd_sendto(ftps_sock, (char *)&trollPacket.packet, sizeof(trollPacket.packet), 0, &ftps_datagram, sizeof(ftps_datagram));

	tcpd_recvfrom(tcpds_sock, (char *)&tcpdsPacket, sizeof(tcpdsPacket), 0, &datagram, &datagram_len);

	printf("Received ACK from ftps, forwarding to tcpdc\n");
	tcpd_sendto(tcpdc_sock, (char *)&tcpdsPacket, sizeof(tcpdsPacket), 0, &tcpdc_datagram, sizeof(tcpdc_datagram));
}

/* tcp daemon called with host name and port number of server */
int main (int argc, char *argv[]) 
{
    if (argc > 1)
	{
	  printf("Usage -- tcpds\n");
	  exit(1);
	}

	/* Create sockets, create packets, etc. */
	initializeData();


	printf("TCPDS listening on port %d\n", ntohs(datagram.sin_port));
	printf("TCPDS sending to FTPS on port %d\n", ntohs(ftps_datagram.sin_port));
	printf("TCPDS forward acks to TCPDC on port %d\n", ntohs(tcpdc_datagram.sin_port));

	/* Send SYN packets to ftps and forward ACK to tcpdc */
	establishConnection();

	int bytes_recv = MAX_BUF_SIZE;
	int bytes_sent = 0;
	
	fd_set fds;
	int result = 0;

	/* Receive image data until the FIN bit has been set */
    while ( (trollPacket.packet.flags & 0x1) == 0) 
	{
		/* receives packets from troll process */
		bytes_recv = tcpd_recvfrom(tcpds_sock, (char *)&trollPacket, sizeof(trollPacket), 0, &datagram, &datagram_len);

		/* compute the checksum and compare to packet header value */
		uint16_t crc = checksum(trollPacket.packet.data);
		printf("server checksum = %04x, packet checksum = %04x\n", crc, trollPacket.packet.checksum);
		
		if (crc == trollPacket.packet.checksum)
		{
			/*forward buffer message from daemon to ftps*/
			bytes_sent = tcpd_sendto(ftps_sock, (char *)&trollPacket.packet, sizeof(trollPacket.packet), 0, &ftps_datagram, sizeof(ftps_datagram));

			printf("Forwarding sequence num %u to ftps\n", trollPacket.packet.seqNum);

			/* If we receive a FIN packet, forward it to ftps and close the connection */
			if ( (trollPacket.packet.flags & 0x1) == 1)
				break;

			FD_ZERO(&fds);
			FD_SET(tcpds_sock, &fds);
			result = select(sizeof(fds) * 8, &fds, NULL, NULL, &timeout);
			if (result == 0)
			    continue;

			bytes_recv = tcpd_recvfrom(tcpds_sock, (char *)&tcpdsPacket, sizeof(tcpdsPacket), 0, &datagram, &datagram_len);
			
			printf("Forwarding ackNum %u to tcpdc\n", tcpdsPacket.ackNum);

			bytes_sent = tcpd_sendto(tcpdc_sock, (char *)&tcpdsPacket, sizeof(tcpdsPacket), 0, &tcpdc_datagram, sizeof(tcpdc_datagram)); 

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
