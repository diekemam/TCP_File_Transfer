#include "tcpd_functions.h"
#include "libs.h"

int tcpd_socket(int domain, int type, int protocol)
{
    int result = socket(domain, type, protocol);
    if (result < 0)
    {
	    perror("Error opening datagram socket\n");
	    exit(1);
    }
    return result;
}

void tcpd_bind(int sock, void *datagram, int datagramSize)
{
    int result = bind(sock, (struct sockaddr *)datagram, datagramSize);
    if (result < 0)
	{
	    perror("Error binding datagram socket\n");
	    exit(2);
	}
}

int tcpd_sendto(int sock, void *msg, int len, unsigned int flags, void *to, int tolen)
{
    int bytes_sent = sendto(sock, msg, len, flags, (struct sockaddr *)to, tolen);
    if (bytes_sent < 0)
	{
	    perror("Error writing on socket\n");
	    exit(3);
	}
	return bytes_sent;
}

int tcpd_recvfrom(int sock, void *msg, int len, unsigned int flags, void *from, int *fromlen)
{
    int bytes_recv = recvfrom(sock, msg, len, flags, (struct sockaddr *)from, fromlen);
    if (bytes_recv < 0)
	{
	    perror("Error reading from socket\n");
	    exit(4);
	}
    return bytes_recv;
}
