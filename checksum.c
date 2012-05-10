/* Checksum function using 16 bit CRC algorithm */

#include "libs.h"
#include "packet.h"
#include "checksum.h"

/* Takes the data buffer as its argument and returns the 16 bit checksum value */
uint16_t checksum(char buf[MAX_BUF_SIZE]) {

	uint16_t crc = 0;
	uint8_t data[MAX_BUF_SIZE];
	uint16_t generator = 0x1021;  /* represents the polynomial x^16+x^12+x^5+1 */
	int current_byte = 0, shift_count = 0;
	
	/* Convert character data into unsigned 8-bit integers for bit shifting operations */
	memcpy(data, buf, MAX_BUF_SIZE);
	
	/* Iterate over each byte of data */
	while (current_byte < MAX_BUF_SIZE)
	{
		/* Iterate over each bit in the current byte of data */
		uint8_t data_byte = data[current_byte];
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
	}
	
	return crc;
}
