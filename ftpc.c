/* 
 * ftpc.c, File Transfer Client using UDP
 * Modified from client.c, UDP version
 * Adam Zink, 4/24/12
 * 
 * Run ftpc on mu.cse.ohio-state.edu
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
#define SRV_PORT 30700
#define TROLL_PORT 30701
#define SRV_HOST_NAME "kappa.cse.ohio-state.edu"
#define LOC_HOST_NAME "mu.cse.ohio-state.edu"

/*
 * The file transfer client takes a filename to transfer as its only argument.  It opens
 * a UDP socket, creates a message for the server, packs the message within a packet
 * that is sent to the troll, opens the specified file, and sends MTU sized data buffers
 * to the server process.
 */
int main(int argc, char *argv[])
{
    int sock;
    struct sockaddr_in name;
	struct {
		struct sockaddr_in header;
		int body_len;
		char body[MAX_BUF_SIZE];
	} message;
    struct hostent *hp, *lp, *gethostbyname();

    if (argc < 2) {
		printf("usage: ftpc filename\n");
		exit(1);
    }
	char filename[MAX_BUF_SIZE];
	strcpy(filename, argv[1]);

    /* create socket for connecting to server */
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
		perror("opening datagram socket");
		exit(2);
    }

    /* construct name for connecting to troll */
    name.sin_family = AF_INET;
    name.sin_port = htons(TROLL_PORT);

    /* convert troll hostname to IP address and enter into name */
    lp = gethostbyname(LOC_HOST_NAME);
    if (lp == 0) {
		fprintf(stderr, "%s:unknown host\n", LOC_HOST_NAME);
		exit(3);
    }
    bcopy((char *)lp->h_addr, (char *)&name.sin_addr, lp->h_length);
	
	
	/* prepare message for troll */
	message.header.sin_family = htons(AF_INET);
	message.header.sin_port = htons(SRV_PORT);
	
	/* convert server hostname to IP address and enter into message */
    hp = gethostbyname(SRV_HOST_NAME);
    if (hp == 0) {
		fprintf(stderr, "%s:unknown host\n", SRV_HOST_NAME);
		exit(3);
    }
    bcopy((char *)hp->h_addr, (char *)&message.header.sin_addr, hp->h_length);

	
	/* open file to transfer */
	FILE *fp;
	fp = fopen(filename, "rb");
	if (!fp) {
		perror("error opening file to transfer");
		exit(4);
	}
	
	/* put filename in body of message to server */
	strcpy(message.body, argv[1]);
	
	/* write filename to sock */
	if (sendto(sock, (char *)&message, sizeof message, 0, (struct sockaddr *)&name, sizeof(name)) < 0) {
		perror("error writing on socket");
		exit(5);
	}
	printf("Client sending filename: %s\n", message.body);
	printf("Sending file...\n");
	
	int bytes_read = 1;
	int bytes_sent = 0;
	int bytes_total = 0;
	while (bytes_read > 0) {
	  
		/* sleep ~10 ms to space packets received by troll */
		int i;
		for(i=0; i<500000; i++);
		
		/* read part of file into buf */
		bzero(message.body, MAX_BUF_SIZE);
		bytes_read = fread(message.body, 1, MAX_BUF_SIZE, fp);
		message.body_len = bytes_read;
		
		/* write buf to sock */
		bytes_sent = sendto(sock, (char *)&message, sizeof message, 0, (struct sockaddr *)&name, sizeof(name));
		if(bytes_sent < 0) {
			perror("error writing on socket");
			exit(6);
		}
		bytes_total += bytes_read;
		printf("%d bytes read,\ttotal: %d bytes\n", bytes_read, bytes_total);
	}
	printf("Finished sending %s\n", filename);
	fclose(fp);

    /* close connection */
    close(socket);
    exit(0);
	return 0;
}
