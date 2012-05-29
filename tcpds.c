/* 
 * tcpds.c, TCP Daemon on server side using UDP functions
 * Michael Diekema, Adam Zink
 */

#include "libs.h"
#include "file_transfer.h"
#include "packet.h"
#include "tcpd_functions.h"
#include "checksum.h"
#include "deltatimer.h"
#include "tcpd_buf.h"

int datagram_len, in_troll_sock, in_ftps_sock, out_sock;
struct sockaddr_in from_troll_datagram, from_ftps_datagram, to_ftps_datagram, to_tcpdc_datagram;
struct hostent *clientAddress, *serverAddress, *gethostbyname();
struct timeval timeout;
Troll_Packet trollPacket;
TCP_Packet tcpdsPacket;
fd_set fds;

void initializeData()
{
	datagram_len = sizeof(struct sockaddr_in);

	/* Create sockets for receiving datagrams, sending to ftps, and sending to tcpdc */
	in_troll_sock = tcpd_socket(AF_INET, SOCK_DGRAM, 0);
	in_ftps_sock = tcpd_socket(AF_INET, SOCK_DGRAM, 0);
	out_sock = tcpd_socket(AF_INET, SOCK_DGRAM, 0);

	/* Construct datagram for connecting to ftps */
	to_ftps_datagram.sin_family = AF_INET;
	to_ftps_datagram.sin_port = htons(FTPS_PORT);

	/* Construct datagram for forwarding acks to tcpdc */
	to_tcpdc_datagram.sin_family = AF_INET;
	to_tcpdc_datagram.sin_port = htons(TCPDC_FROM_TCPDS_PORT);

	/* Construct datagram for receiving from troll */
	from_troll_datagram.sin_family = AF_INET;
	from_troll_datagram.sin_port = htons(TCPDS_FROM_TROLL_PORT);
	from_troll_datagram.sin_addr.s_addr = INADDR_ANY;

	/* Construct datagram for receiving from ftps */
	from_ftps_datagram.sin_family = AF_INET;
	from_ftps_datagram.sin_port = htons(TCPDS_FROM_FTPS_PORT);
	from_ftps_datagram.sin_addr.s_addr = INADDR_ANY;

	/* Convert tcpdc hostname to IP address and enter into to_tcpdc_datagram */
    clientAddress = gethostbyname(CLI_HOST_NAME);
	if (clientAddress == 0) 
	{
	    fprintf(stderr, "%s:unknown host\n", CLI_HOST_NAME);
		exit(3);
	}
    bcopy((char *)clientAddress->h_addr, (char *)&to_tcpdc_datagram.sin_addr, clientAddress->h_length);

	/* Convert ftps hostname to IP address and enter into to_ftps_datagram */
    serverAddress = gethostbyname(SRV_HOST_NAME);
	if (serverAddress == 0) 
	{
	    fprintf(stderr, "%s:unknown host\n", SRV_HOST_NAME);
		exit(3);
	}
    bcopy((char *)serverAddress->h_addr, (char *)&to_ftps_datagram.sin_addr, serverAddress->h_length);

	printf("Sending to ftps on port %d\n", to_ftps_datagram.sin_port);
	printf("Sending to tcpdc on port %d\n", to_tcpdc_datagram.sin_port);
	printf("Receiving from troll on port %d\n", from_troll_datagram.sin_port);
	printf("Receiving from ftps on port %d\n", from_ftps_datagram.sin_port);

	/* Bind input sockets, so they're listening to the correct source address and port number */
	tcpd_bind(in_troll_sock, &from_troll_datagram, datagram_len);
	tcpd_bind(in_ftps_sock, &from_ftps_datagram, datagram_len);

	/* Set timeout to .1 seconds */
	timeout.tv_sec = 0;
	timeout.tv_usec = 100000;
}

void establishConnection()
{
	/* Wait indefinitely until we receive a SYN packet from the troll */
	tcpd_recvfrom(in_troll_sock, (char *)&trollPacket, sizeof(trollPacket), 0, &from_troll_datagram, &datagram_len);

	printf("Received SYN packet from troll, forwarding to ftps\n");

	tcpd_sendto(out_sock, (char *)&trollPacket.packet, sizeof(trollPacket.packet), 0, &to_ftps_datagram, datagram_len);

	tcpd_recvfrom(in_ftps_sock, (char *)&tcpdsPacket, sizeof(tcpdsPacket), 0, &from_ftps_datagram, &datagram_len);

	printf("Received ACK from ftps, forwarding to tcpdc\n");
	tcpd_sendto(out_sock, (char *)&tcpdsPacket, sizeof(tcpdsPacket), 0, &to_tcpdc_datagram, datagram_len);
}

void sendFile()
{
    /* Set the timeout to 2 seconds */
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;

	int bytes_sent, bytes_recv, done = 0;
	while (!done)
	{
	    /* Wait until we hear from ftpc or tcpds */
	    FD_ZERO(&fds);
	    FD_SET(in_troll_sock, &fds);
	    FD_SET(in_ftps_sock, &fds);
	    int result = select(FD_SETSIZE, &fds, NULL, NULL, &timeout);
		if (result < 0)
		{
			fprintf(stderr, "Error: select failed\n");
			exit(4);
		}

		/* Keep waiting until we hear from ftpc or tcpds */
		if (result == 0)
		    continue;

		if (FD_ISSET(in_troll_sock, &fds))
		{
		    /* receives packets from troll process */
		    bytes_recv = tcpd_recvfrom(in_troll_sock, (char *)&trollPacket, sizeof(trollPacket), 0, &from_troll_datagram, &datagram_len);

			/* compute the checksum and compare to packet header value */
			uint16_t crc = checksum(trollPacket.packet.data);
			printf("server checksum = %04x, packet checksum = %04x\n", crc, trollPacket.packet.checksum);
		
			if (crc == trollPacket.packet.checksum)
			{
			    printf("Forwarding sequence num %u to ftps\n", trollPacket.packet.seqNum);

			    /*forward buffer message from daemon to ftps*/
			    bytes_sent = tcpd_sendto(out_sock, (char *)&trollPacket.packet, sizeof(trollPacket.packet), 0, &to_ftps_datagram, datagram_len);

				/* If we receive a FIN packet, forward it to ftps and close the connection */
				if ( (trollPacket.packet.flags & 0x1) == 1)
				    done = 1;
			}

			else
			{
			    /* invalid checksum, dropping packet */
			    printf("Packet dropped\n");
			}
		}
		
		else if (FD_ISSET(in_ftps_sock, &fds))
		{
		    bytes_recv = tcpd_recvfrom(in_ftps_sock, (char *)&tcpdsPacket, sizeof(tcpdsPacket), 0, &from_ftps_datagram, &datagram_len);
			
			printf("Forwarding ackNum %u to tcpdc\n", tcpdsPacket.ackNum);

			bytes_sent = tcpd_sendto(out_sock, (char *)&tcpdsPacket, sizeof(tcpdsPacket), 0, &to_tcpdc_datagram, datagram_len);
		}
	}
}

int main (int argc, char *argv[]) 
{
    if (argc > 1)
	{
	  printf("Usage -- tcpds\n");
	  exit(1);
	}

	/* Create sockets, create packets, etc. */
	initializeData();

	/* Send SYN packets to ftps and forward ACK to tcpdc */
	establishConnection();

	/* Receive file from troll using circular buffer and forward packet to ftps */
	sendFile();
	printf("Finished forwarding file to ftps\n");
	return 0;
}
