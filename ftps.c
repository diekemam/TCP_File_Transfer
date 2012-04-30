/* 
 * ftps.c, File Transfer Server using UDP
 * Modified from server.c, UDP version
 * Adam Zink, 4/24/12
 * 
 * Run ftps on kappa.cse.ohio-state.edu
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
#define RECV_PORT 30700

/*
 * The file transfer server takes no arguments - it simply waits to receive data from 
 * the sender on a predefined port.  It opens a UDP socket, opens a new file to write,
 * creates a message to accept incoming data messages from the client, and receives data
 * until getting an empty packet, indicating the end of the file.
 */
int main (void)
{
    int sock, namelen;
    struct sockaddr_in name;
	char buf[MAX_BUF_SIZE];

    /* Open a UDP socket */
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
		perror("opening datagram socket");
		exit(1);
    }

    /* create name with parameters and bind name to socket */
    name.sin_family = AF_INET;
    name.sin_port = htons(RECV_PORT);
    name.sin_addr.s_addr = INADDR_ANY;
    if(bind(sock, (struct sockaddr *)&name, sizeof(name)) < 0) {
		perror("getting socket name");
		exit(2);
    }
    namelen = sizeof(struct sockaddr_in);
	
    /* Find assigned port value and print it for client to use */
    if(getsockname(sock, (struct sockaddr *)&name, &namelen) < 0){
		perror("getting sock name");
		exit(3);
    }
    printf("Server waiting on port # %d\n", ntohs(name.sin_port));

	/* open file to write */
	FILE *fp;
	fp = fopen("MyImage1.jpg", "wb");
	if (!fp) {
		perror("error opening file to transfer");
		exit(5);
	}
	
	int bytes_recv = MAX_BUF_SIZE;
	int bytes_written = 0;
	int bytes_total = 0;
	while (bytes_recv > 0) {
  
		/* read from sock and place in buf */
		bzero(buf, MAX_BUF_SIZE);
		bytes_recv = recvfrom(sock, buf, sizeof buf, 0, (struct sockaddr *)&name, &namelen);
		if(bytes_recv < 0) {
			perror("error reading on socket");
			exit(6);
		}
		
		/* write received message to file */
		if (bytes_recv > 0) {
			bytes_written = fwrite(buf, 1, bytes_recv , fp);
			bytes_total += bytes_written;
		}
		printf("%d bytes received,\t%d bytes written,\ttotal: %d bytes\n", bytes_recv, bytes_written, bytes_total);
		bytes_written = 0;
	}
	printf("Finished writing MyImage1.jpg\n");
	fclose(fp);

    /* server terminates connection, closes socket, and exits */
    close(sock);
    exit(0);
	return 0;
}
