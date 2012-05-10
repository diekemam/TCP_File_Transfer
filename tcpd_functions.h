#ifndef TCPD_FUNCTIONS_H
#define TCPD_FUNCTIONS_H

extern int tcpd_socket(int domain, int type, int protocol);
extern void tcpd_bind(int sock, void *datagram, int datagramSize);
extern int tcpd_sendto(int sock, void *msg, int len, unsigned int flags, void *to, int tolen);
extern int tcpd_recvfrom(int sock, void *msg, int len, unsigned int flags, void *from, int *fromlen);

#endif
