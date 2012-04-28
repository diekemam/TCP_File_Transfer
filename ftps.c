/* 
 * ftps.c, File Transfer Server using UDP
 * Modified from server.c, UDP version
 * Adam Zink, 4/24/12
 * 
 * Run ftps on kappa.cse.ohio-state.edu
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#define MAX_BUF_SIZE 980
#define RECV_PORT 30700

/*
 * The file transfer server takes no arguments - it simply waits to receive data from 
 * the sender on a predefined port.  It opens a UDP socket, opens a new file to write,
 * creates a message to accept incoming data messages from the client, and receives data
 * until getting an empty packet, indicating the end of the file.
 */
main (void)
{
    int sock, namelen;
    struct sockaddr_in name;
	struct {
		struct sockaddr_in header;
		int body_len;
		char body[MAX_BUF_SIZE];
	} message;
	/*char filename[MAX_BUF_SIZE];*/

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

	/* read filename from msgsock and place in buf */
	/*bzero(filename, MAX_BUF_SIZE);*/
	if(recvfrom(sock, (char *)&message, sizeof message, 0, (struct sockaddr *)&name, &namelen) < 0) {
		perror("error reading on socket");
		exit(4);
	}
	/*strcpy(filename, message.body);
	printf("Server receiving filename: %s\n", filename);*/
	
	/* create new name for copied file */
	/* old:
	char write_filename[MAX_BUF_SIZE];
	bzero(write_filename, MAX_BUF_SIZE);
	strcat(write_filename, "copy_of_");
	strcat(write_filename, filename);
	printf("Writing file: %s\n", write_filename);
	printf("Ready to receive file...\n");
	*/

	/* open file to write */
	FILE *fp;
	fp = fopen("copiedFile.jpg", "wb");
	if (!fp) {
		perror("error opening file to transfer");
		exit(5);
	}
	
	int bytes_recv = 0;
	int bytes_in_msg = 1;
	int bytes_written = 0;
	int bytes_total = 0;
	while (bytes_in_msg > 0) {
  
		/* read from sock and place in buf */
		bzero(message.body, MAX_BUF_SIZE);
		bytes_recv = recvfrom(sock, (char *)&message, sizeof message, 0, (struct sockaddr *)&name, &namelen);
		bytes_in_msg = message.body_len;
		if(bytes_recv < 0) {
			perror("error reading on socket");
			exit(6);
		}
		
		/* write received message to file */
		if (bytes_in_msg > 0) {
			bytes_written = fwrite(message.body, 1, message.body_len, fp);
			bytes_total += bytes_written;
		}
		printf("%d bytes in msg,\t%d bytes written,\ttotal: %d bytes\n", bytes_in_msg, bytes_written, bytes_total);
		bytes_written = 0;
	}
	printf("Finished writing %s\n", write_filename);
	fclose(fp);

    /* server terminates connection, closes socket, and exits */
    close(sock);
    exit(0);
}
