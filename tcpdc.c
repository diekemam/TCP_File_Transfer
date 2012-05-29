/* 
 * tcpdc.c, TCP Daemon on client side using UDP functions
 * Michael Diekema, Adam Zink
*/

#include "libs.h"
#include "file_transfer.h"
#include "packet.h"
#include "tcpd_functions.h"
#include "checksum.h"
#include "deltatimer.h"
#include "tcpd_buf.h"

int datagram_len, in_ftpc_sock, in_tcpds_sock, out_sock;
struct sockaddr_in from_tcpds_datagram, from_ftpc_datagram, to_ftpc_datagram, to_troll_datagram;
struct hostent *clientAddress, *serverAddress, *gethostbyname();
struct timeval timeout;
Troll_Packet trollPacket;
TCP_Packet tcpdcPacket; /* Used to receive acks from server */
fd_set fds;

void initializeData()
{
    datagram_len = sizeof(struct sockaddr_in);

    /* create socket for receiving from ftpc and tcpds, and sending to troll and ftpc */
	in_ftpc_sock = tcpd_socket(AF_INET, SOCK_DGRAM, 0);
	in_tcpds_sock = tcpd_socket(AF_INET, SOCK_DGRAM, 0);
	out_sock = tcpd_socket(AF_INET, SOCK_DGRAM, 0);

	/* construct datagram for connecting to troll */
	to_troll_datagram.sin_family = AF_INET;
	to_troll_datagram.sin_port = htons(TROLL_CLIENT_PORT);

	/* construct datagram for forwarding acks to ftpc */
	to_ftpc_datagram.sin_family = AF_INET;
	to_ftpc_datagram.sin_port = htons(FTPC_PORT);

	/* construct datagram for receiving from tcpds */
	from_tcpds_datagram.sin_family = AF_INET;
	from_tcpds_datagram.sin_port = htons(TCPDC_FROM_TCPDS_PORT);
	from_tcpds_datagram.sin_addr.s_addr = INADDR_ANY;

	/* construct datagram for receiving from ftpc */
	from_ftpc_datagram.sin_family = AF_INET;
	from_ftpc_datagram.sin_port = htons(TCPDC_FROM_FTPC_PORT);
	from_tcpds_datagram.sin_addr.s_addr = INADDR_ANY;

	/* prepare message for troll */
	trollPacket.header.sin_family = htons(AF_INET);
	trollPacket.header.sin_port = htons(TCPDS_FROM_TROLL_PORT);

	/* convert ftpc hostname to IP address and enter into ftpc_datagram and troll_datagram */
	clientAddress = gethostbyname(CLI_HOST_NAME);
	if (clientAddress == 0) 
	{
		fprintf(stderr, "%s:unknown host\n", CLI_HOST_NAME);
		exit(3);
	}
	bcopy((char *)clientAddress->h_addr, (char *)&to_ftpc_datagram.sin_addr, clientAddress->h_length);
	bcopy((char *)clientAddress->h_addr, (char *)&to_troll_datagram.sin_addr, clientAddress->h_length);
			
	/* convert server hostname to IP address and enter into troll_datagram */
	serverAddress = gethostbyname(SRV_HOST_NAME);
	if (serverAddress == 0)
    {
		fprintf(stderr, "%s:unknown host\n", SRV_HOST_NAME);
		exit(3);
	}
	bcopy((char *)serverAddress->h_addr, (char *)&trollPacket.header.sin_addr, serverAddress->h_length);

	printf("Sending to ftpc on port %d\n", to_ftpc_datagram.sin_port);
	printf("Sending to troll on port %d\n", to_troll_datagram.sin_port);
	printf("Receiving from ftpc on port %d\n", from_ftpc_datagram.sin_port);
	printf("Receiving from tcpds on port %d\n", from_tcpds_datagram.sin_port);

	/* Bind input sockets, so they're listening to the correct source address and port number */
	tcpd_bind(in_ftpc_sock, &from_ftpc_datagram, datagram_len);
	tcpd_bind(in_tcpds_sock, &from_tcpds_datagram, datagram_len);

	/* Set timeout to .1 seconds */
	timeout.tv_sec = 0;
	timeout.tv_usec = 100000;
}

void establishConnection()
{
	int result = 0;
	do
	{
	    tcpd_recvfrom(in_ftpc_sock, (char *)&tcpdcPacket, sizeof(tcpdcPacket), 0, &from_ftpc_datagram, &datagram_len);
	    printf("Received SYN packet from client, forwarding to tcpds\n");
	    tcpd_sendto(out_sock, (char *)&trollPacket, sizeof(trollPacket), 0, &to_troll_datagram, datagram_len);
		
	    FD_ZERO(&fds);
	    FD_SET(in_tcpds_sock, &fds);
	    result = select(FD_SETSIZE, &fds, NULL, NULL, &timeout);

		/* If we have not received an ACK, ftpc will retransmit the SYN packet */
		if (result == 0)
		    continue;

	    tcpd_recvfrom(in_tcpds_sock, (char *)&tcpdcPacket, sizeof(tcpdcPacket), 0, &from_tcpds_datagram, &datagram_len);
	  
	} while(result == 0);

	/* Now we have received the ACK, so send it back to ftpc */
	printf("Received SYNACK, forwarding to ftpc\n");
	tcpd_sendto(out_sock, (char *)&tcpdcPacket, sizeof(tcpdcPacket), 0, &to_ftpc_datagram, datagram_len);
}

void sendFile()
{
    /* Set the timeout to 2 seconds */
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;

	int bytes_sent, bytes_recv;

	while (1)
	{
	    /* Wait until we hear from ftpc or tcpds */
	    FD_ZERO(&fds);
	    FD_SET(in_ftpc_sock, &fds);
	    FD_SET(in_tcpds_sock, &fds);
	    int result = select(FD_SETSIZE, &fds, NULL, NULL, &timeout);
		if (result < 0)
		{
			fprintf(stderr, "Error: select failed\n");
			exit(4);
		}

		/* Keep waiting until we hear from ftpc or tcpds */
		if (result == 0)
		    continue;

		if (FD_ISSET(in_ftpc_sock, &fds))
		{
		    /* Receives packets from local ftpc process */
		    bzero(&trollPacket.packet, sizeof(trollPacket.packet));

			bytes_recv = tcpd_recvfrom(in_ftpc_sock, (char *)&trollPacket.packet, sizeof(trollPacket.packet), 0, &from_ftpc_datagram, &datagram_len);

			/* Compute checksum before forwarding packet to troll */
			trollPacket.packet.checksum = checksum(trollPacket.packet.data);
			printf("client checksum = %04x\n", trollPacket.packet.checksum);

			printf("Forwarding sequence num %u to tcpds through troll\n", trollPacket.packet.seqNum);

			/* Forward buffer message from daemon to troll process */
			bytes_sent = tcpd_sendto(out_sock, (char *)&trollPacket, sizeof(trollPacket), 0, &to_troll_datagram, datagram_len);

			/* End the connection once the FIN bit is set */ 
			if ( (trollPacket.packet.flags & 0x1) == 1)
			    break;
		}

		else if (FD_ISSET(in_tcpds_sock, &fds))
		{
		    bzero(&tcpdcPacket, sizeof(tcpdcPacket));

			bytes_recv = tcpd_recvfrom(in_tcpds_sock, (char *)&tcpdcPacket, sizeof(tcpdcPacket), 0, &from_tcpds_datagram, &datagram_len);

			printf("Forwarding ackNum %u to ftpc\n", tcpdcPacket.ackNum);

			bytes_sent = tcpd_sendto(out_sock, (char *)&tcpdcPacket, sizeof(tcpdcPacket), 0, &to_ftpc_datagram, datagram_len);
		}
	}
}

int main (int argc, char *argv[]) 
{
    if (argc > 1)
	{
	  printf("Usage -- tcpdc\n");
	  exit(1);
	}

	/* Create sockets, create packets, etc. */
	initializeData();

	/* Send the SYN packets over to tcpds to start the connection */
	establishConnection(); 

	/* Send file to tcpds using circular buffer and forward incoming ACKs to ftpc */
	sendFile();
	printf("Finished forwarding file\n");
	return(0);
}
