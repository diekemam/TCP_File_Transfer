#include "libs.h"
#include "packet.h"
#include "file_transfer.h"
#include "tcpd_buf.h"

/* Start and end of the circular buffer */
unsigned bufHead = 0, bufTail = 0;

/* Used to allow entire circular buffer to be filled -- packets can be
 * added if numBufElements < TCPD_BUF_SIZE */
int numBufElements = 0;

int tcpd_buf_add(TCP_Packet packet)
{
    if (numBufElements >= TCPD_BUF_SIZE)
	    return -1;
	crcBuf[bufTail] = packet;
    bufTail = (bufTail + 1) % TCPD_BUF_SIZE;
    numBufElements++;
    return 0;
}

int tcpd_buf_rmv(int numElements)
{
    if (numBufElements < numElements)
	    return -1;
    bufHead = (bufHead + numElements) % TCPD_BUF_SIZE;
    numBufElements -= numElements;
    return 0;
}

int is_tcpd_buf_full()
{
    return numBufElements == TCPD_BUF_SIZE;
}
