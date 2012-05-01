/* 
 * tcpd.c, TCP Daemon using UDP functions
 * Adam Zink, 4/24/12
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

#define MAX_BUF_SIZE 980
#define CLI_HOST_NAME "mu.cse.ohio-state.edu"
#define SRV_HOST_NAME "kappa.cse.ohio-state.edu"
#define FTPS_PORT 30700
#define TCPDC_PORT 30701
#define TROLL_PORT 30702
#define TCPDS_PORT 30703

/* tcp daemon called with host name and port number of server */
int main (int argc, char *argv[]) 
{

	if (argc < 2) 
	{
		printf("usage: tcpd (c)lient-or-(s)erver\n");
		exit(1);
    }

	int datagram_len = sizeof(struct sockaddr_in);
	int bytes_recv = MAX_BUF_SIZE;
	int bytes_in_msg = MAX_BUF_SIZE;
	int bytes_sent = 0;

	int ftpc_sock, ftps_sock, tcpds_sock, troll_sock;
	struct sockaddr_in datagram, tcpdc_datagram, ftps_datagram;

	struct
	{
		struct sockaddr_in header;
		int body_len;
		char body[MAX_BUF_SIZE];
	} message;
	
	struct hostent *hp, *lp, *gethostbyname();

	/* Server/client choice is the first character of the second argument -- we'll probably remove this later and implement tcpdc.c and tcpds.c */
	char choice = argv[1][0];
	
	switch (choice) 
	{
		case 's':

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
		
		
			break;


	case 'c':
			
	  printf("Reached TCPDC\n");
			/* create socket for receiving from ftpc */
			ftpc_sock = socket(AF_INET, SOCK_DGRAM, 0);
			if (ftpc_sock < 0) 
			{
				perror("opening datagram socket");
				exit(2);
			}
			
			/* create socket for sending to troll */
			troll_sock = socket(AF_INET, SOCK_DGRAM, 0);
			if (troll_sock < 0) 
			{
				perror("opening datagram socket");
				exit(2);
			}

			/* construct datagram for receiving from ftpc */
			datagram.sin_family = AF_INET;
			datagram.sin_port = htons(TCPDC_PORT);
			datagram.sin_addr.s_addr = INADDR_ANY;

			if (bind(ftpc_sock, (struct sockaddr *)&datagram, sizeof(datagram)) < 0)
			{
				perror("error binding datagram socket\n");
				exit(2);
			}
			printf("TCPDC waiting on port num %d\n", ntohs(datagram.sin_port));

			/* construct datagram for connecting to TCPDS */
			tcpdc_datagram.sin_family = AF_INET;
			tcpdc_datagram.sin_port = htons(TCPDS_PORT);

			/* convert troll hostname to IP address and enter into name */
			lp = gethostbyname(CLI_HOST_NAME);
			if (lp == 0) 
			{
				fprintf(stderr, "%s:unknown host\n", CLI_HOST_NAME);
				exit(3);
			}
			bcopy((char *)lp->h_addr, (char *)&message.header.sin_addr, lp->h_length);
			
			/* prepare message for troll */
			message.header.sin_family = htons(AF_INET);
			message.header.sin_port = htons(TROLL_PORT);
			
			/* convert server hostname to IP address and enter into tcpdc_datagram */
			hp = gethostbyname(SRV_HOST_NAME);
			if (hp == 0) {
				fprintf(stderr, "%s:unknown host\n", SRV_HOST_NAME);
				exit(3);
			}
			bcopy((char *)hp->h_addr, (char *)&tcpdc_datagram.sin_addr, hp->h_length);
			printf("Sending to troll at port %d\n", ntohs(message.header.sin_port));
			printf("Troll sending to TCPDS at port %d\n", ntohs(tcpdc_datagram.sin_port));

			while (bytes_recv > 0) 
			{
				/* receives packets from local ftpc process */
				bzero(message.body, MAX_BUF_SIZE);

				bytes_recv = recvfrom(ftpc_sock, (char *)&message.body, sizeof message.body, 0, (struct sockaddr *)&datagram, &datagram_len);

				message.body_len = bytes_recv;
				if(bytes_recv < 0) {
					perror("error reading on socket");
					exit(6);
				}
			  
				/* forward buffer message from daemon to troll process */
				bytes_sent = sendto(troll_sock, (char *)&message, sizeof message, 0, (struct sockaddr *)&tcpdc_datagram, sizeof(tcpdc_datagram));
				if(bytes_sent < 0) {
					perror("error writing on socket");
					exit(6);
				}
				
			}
			printf("Finished forwarding file\n");
			break;

		default:
			printf("usage: tcpd (c)lient-or-(s)erver\n");
			exit(1);
	}

	exit(0);
	return(0);
}
