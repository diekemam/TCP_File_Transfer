#ifndef DELTATIMER_H
#define DELTATIMER_H

#include <sys/types.h>
struct deltaTime
{
	unsigned seconds;
	unsigned microSeconds;
};

struct deltaTimer
{
    struct deltaTime time;
    unsigned portNum;
    unsigned seqNum;
};

struct node
{
    struct deltaTimer timer;
    struct node *next;
    struct node *prev;
};

void addBeg(struct deltaTimer value);
void addEnd(struct deltaTimer value);
int delBeg();
int delSeqNum(uint32_t seqNum);
void printList();

#endif
