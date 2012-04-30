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
#define FTPC_PORT 30701
#define TROLL_PORT 30702
#define TCPDS_PORT 30703

/* tcp daemon called with host name and port number of server */
int main (int argc, char *argv[]) {

	if (argc < 2) {
		printf("usage: tcpd (c)lient-or-(s)erver\n");
		exit(1);
    }

	int datagram_len = sizeof(struct sockaddr_in);
	int bytes_recv = 0;
	int bytes_in_msg = MAX_BUF_SIZE;
	int bytes_sent = 0;



	int ftpc_sock, ftps_sock, troll_sock;
	struct sockaddr_in datagram, ftps_datagram;

	struct
	{
		struct sockaddr_in header;
		int body_len;
		char body[MAX_BUF_SIZE];
	} message;
	
	struct hostent *hp, *lp, *gethostbyname();
	
	/* Server/client choice is the first character of the second argument -- we'll probably remove this later and implement tcpdc.c and tcpds.c */
	char choice = argv[1][0];
	
	switch (choice) {
		case 's':

		    /* construct name for listening in from tcpds */
			datagram.sin_family = AF_INET;
			datagram.sin_port = htons(TCPDS_PORT);

			/* convert troll hostname to IP address and enter into name */
			lp = gethostbyname(CLI_HOST_NAME);
			if (lp == 0) {
				fprintf(stderr, "%s:unknown host\n", CLI_HOST_NAME);
				exit(3);
			}
			bcopy((char *)lp->h_addr, (char *)&datagram.sin_addr, lp->h_length);

			/* construct name for sending to ftps */
			ftps_datagram.sin_family = AF_INET;
			ftps_datagram.sin_port = htons(FTPS_PORT);

			/* convert troll hostname to IP address and enter into name */
			hp = gethostbyname(SRV_HOST_NAME);
			if (hp == 0) {
				fprintf(stderr, "%s:unknown host\n", SRV_HOST_NAME);
				exit(3);
			}
			bcopy((char *)hp->h_addr, (char *)&ftps_datagram.sin_addr, hp->h_length);

            /* create socket for receiving to ftps */
			ftps_sock = socket(AF_INET, SOCK_DGRAM, 0);
			if (ftps_sock < 0) {
				perror("opening datagram socket");
				exit(2);
			}

			/* create socket for sending to troll */
			troll_sock = socket(AF_INET, SOCK_DGRAM, 0);
			if (troll_sock < 0) {
				perror("opening datagram socket");
				exit(2);
			}

			while (bytes_in_msg > 0) {
			  
			  /* receives packets from troll process */
				bytes_recv = recvfrom(troll_sock, (char *)&message, sizeof message, 0, (struct sockaddr *)&datagram, &datagram_len);
				if(bytes_recv < 0) {
					perror("error reading on socket:tcpds\n");
					exit(6);
				}

		   
				/*forward buffer message from daemon to ftps*/
				bzero(message.body, MAX_BUF_SIZE);
				bytes_sent = sendto(ftps_sock, (char *)&message.body, sizeof message.body, 0, (struct sockaddr *)&ftps_datagram, &datagram_len);
				bytes_in_msg = message.body_len;
				if(bytes_sent < 0) {
					perror("error sending on socket:tcpds\n");
					exit(6);
				}
				
			}
		
		
			break;


	case 'c':
			
			
			/* create socket for receiving from ftpc */
			ftpc_sock = socket(AF_INET, SOCK_DGRAM, 0);
			if (ftpc_sock < 0) {
				perror("opening datagram socket");
				exit(2);
			}
			
			/* create socket for sending to troll */
			troll_sock = socket(AF_INET, SOCK_DGRAM, 0);
			if (troll_sock < 0) {
				perror("opening datagram socket");
				exit(2);
			}

			/* construct name for connecting to troll */
			datagram.sin_family = AF_INET;
			datagram.sin_port = htons(TROLL_PORT);

			/* convert troll hostname to IP address and enter into name */
			lp = gethostbyname(CLI_HOST_NAME);
			if (lp == 0) {
				fprintf(stderr, "%s:unknown host\n", CLI_HOST_NAME);
				exit(3);
			}
			bcopy((char *)lp->h_addr, (char *)&datagram.sin_addr, lp->h_length);
			
			
			/* prepare message for troll */
			message.header.sin_family = htons(AF_INET);
			message.header.sin_port = htons(FTPS_PORT);
			
			/* convert server hostname to IP address and enter into message */
			hp = gethostbyname(SRV_HOST_NAME);
			if (hp == 0) {
				fprintf(stderr, "%s:unknown host\n", SRV_HOST_NAME);
				exit(3);
			}
			bcopy((char *)hp->h_addr, (char *)&message.header.sin_addr, hp->h_length);
			
			while (bytes_in_msg > 0) {
			  
				/* receives packets from local ftpc process */
				bzero(message.body, MAX_BUF_SIZE);
				bytes_recv = recvfrom(ftpc_sock, (char *)&message, sizeof message, 0, (struct sockaddr *)&datagram, &datagram_len);
				bytes_in_msg = message.body_len;
				if(bytes_recv < 0) {
					perror("error reading on socket");
					exit(6);
				}
			  
				/* forward buffer message from daemon to troll process */
				bytes_sent = sendto(troll_sock, (char *)&datagram, sizeof datagram, 0, (struct sockaddr *)&datagram, sizeof(datagram));
				if(bytes_sent < 0) {
					perror("error writing on socket");
					exit(6);
				}
				
			}
			printf("Finished forwarding file");
			break;

		default:
			printf("usage: tcpd (c)lient-or-(s)erver\n");
			exit(1);
	}

	exit(0);
	return(0);
}
