#ifndef CSE_678_TCPD_BUF_H
#define CSE_678_TCPD_BUF_H

/* Circular buffer used to send multiple packets through network */
TCP_Packet crcBuf[TCPD_BUF_SIZE];

int tcpd_buf_add(TCP_Packet packet);
int tcpd_buf_rmv(int numPackets);
int is_tcpd_buf_full();
#endif
