#ifndef CSE_678_PACKET_H
#define CSE_678_PACKET_H

#define MAX_BUF_SIZE 980
#define CLI_HOST_NAME "mu.cse.ohio-state.edu"
#define SRV_HOST_NAME "kappa.cse.ohio-state.edu"
#define FTPC_PORT 30699
#define FTPS_PORT 30700
#define TCPDC_PORT 30701
#define TROLL_PORT 30702
#define TCPDS_PORT 30703

typedef struct TCP_Packet
{
	uint32_t seqNum;
	uint32_t ackNum;
	uint8_t flags;
	uint8_t padding;
	uint16_t checksum;
	char data[MAX_BUF_SIZE];
} TCP_Packet;

typedef struct Troll_Packet
{
	struct sockaddr_in header;
	TCP_Packet packet;
} Troll_Packet;

#endif
