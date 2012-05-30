
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>

/* all times stored in microseconds */

uint16_t getRTO(uint16_t *M_rtt, uint16_t *A_rtt, uint16_t *D) {
	uint16_t error = *M_rtt - *A_rtt;
	*A_rtt += (error >> 3);
	*D += ((abs(error) - *D) >> 2);
	return (*A_rtt + (4 * (*D)));
}

int main(void) {
	/* Initialize variables related to RTO calculation */
	uint16_t A = 0;  /* average RTT */
	uint16_t D = 500;  /* smoothed mean deviation */
	uint16_t M = 0;  /* measured RTT */
	uint16_t rto = 0;
	
	/* simulated rtt times */
	uint16_t rtt[] = {2000, 2000, 2500, 2000, 3000};
	
	int rtt_len = sizeof(rtt) / sizeof(uint16_t);
	int i = 0;
	
	/* process simulated rtt's */
	while (i < rtt_len)
	{
		M = rtt[i];
		rto = getRTO(&M, &A, &D);
		
		printf("i=%d,\trtt=%d,\trto=%d,\t[A=%d, D=%d]\n", i, M, rto, A, D);
		i++;
	}
	printf("End of function simulation\n");
	
	return 0;
}
