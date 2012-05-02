/* 
 * tcpdc.c, TCP Daemon on server side using UDP functions
 * Michael Diekema, Adam Zink
 */

#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include "file_transfer.h"

/* tcp daemon called with host name and port number of server */
int main (int argc, char *argv[]) 
{
	int datagram_len = sizeof(struct sockaddr_in);
	int bytes_recv = MAX_BUF_SIZE;
	int bytes_in_msg = MAX_BUF_SIZE;
	int bytes_sent = 0;

	int ftps_sock, tcpds_sock;
	struct sockaddr_in datagram, ftps_datagram;

	struct
	{
		struct sockaddr_in header;
		int body_len;
		char body[MAX_BUF_SIZE];
	} message;
	
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

    while (bytes_in_msg > 0) 
	{
		/* receives packets from troll process */
		bytes_recv = recvfrom(tcpds_sock, (char *)&message, sizeof message, 0, (struct sockaddr *)&datagram, &datagram_len);

		if(bytes_recv < 0) 
		{
			perror("error reading on socket:tcpds\n");
			exit(6);
		}
		   
		/*forward buffer message from daemon to ftps*/
		bytes_sent = sendto(ftps_sock, (char *)&message.body, message.body_len, 0, (struct sockaddr *)&ftps_datagram, sizeof(ftps_datagram));
		bytes_in_msg = message.body_len;

		if(bytes_sent < 0) 
		{
			perror("error sending on socket:tcpds\n");
			exit(6);
		}
	}

	printf("Finished forwarding file to ftps\n");
	return 0;
}
