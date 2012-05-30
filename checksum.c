/* Checksum function using 16 bit CRC algorithm */

#include "libs.h"
#include "packet.h"
#include "checksum.h"

/* Takes the data buffer as its argument and returns the 16 bit checksum value */
uint16_t checksum(TCP_Packet packet) {
	uint16_t temp = packet.checksum;
	packet.checksum = 0;
	uint8_t packet_data[sizeof(struct TCP_Packet)];
	memcpy(packet_data, &packet, sizeof(packet));
	packet.checksum = temp;
	
	uint16_t crc = 0;
	uint16_t generator = 0x1021;  /* represents the polynomial x^16+x^12+x^5+1 */
	int current_byte = 0, shift_count = 0;
	
	/* Iterate over each byte of data */
	while (current_byte < sizeof(struct TCP_Packet))
	{
		/* Iterate over each bit in the current byte of data */
		uint8_t data_byte = packet_data[current_byte];
		/*printf("%02x ", data_byte);*/
		while (shift_count < 8)
		{
			/* shift most significant data bit into least significant bit of crc */
			crc = crc << 1;
			uint16_t next_bit = data_byte >> 7;
			crc = crc | next_bit;
			data_byte = data_byte << 1;
			shift_count++;
			
			if ((crc & 0x8000) != 0x0)  /* if high order bit is 1 */
			{	
				crc = crc ^ generator;  /* xor crc and G */
			} /* nothing to do otherwise */
		}
		shift_count = 0;
		current_byte++;
		/*if (current_byte % 32 == 0) {
			printf("\n");
		}*/
	}
	
	return crc;
}
