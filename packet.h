#ifndef CSE_678_PACKET_H
#define CSE_678_PACKET_H

#include "file_transfer.h"

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
