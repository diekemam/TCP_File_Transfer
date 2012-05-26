#include <stdio.h>
#include <stdlib.h>
#include "deltatimer.h"

struct node *head = NULL;

/* Computes the linked list length by traversing every node in order */
int length()  
{  
    struct node *temp; 
    int count = 0;
    temp = head;

    while(temp != NULL)
    {
        temp = temp -> next;
        count++;
    }
    return(count);
}

/* Adds a new deltaTimer node to the beginning of the list */
void addBeg(struct deltaTimer value)
{
    struct node *temp;
	temp = (struct node *)malloc(sizeof(struct node));
    temp -> timer = value;

	/* If there are no elements in the list, add the first one */
    if (head == NULL)
    {
	    head = temp;
	    head -> next = NULL;
		head -> prev = NULL;
	}

	/* Otherwise, insert the element at the front and point the next element back at it */
    else
	{
	    temp -> next = head;
		head -> prev = temp;
	    head = temp;
	}
}

/* Adds a new deltaTimer node to the end of the list */
void addEnd(struct deltaTimer value)
{
    struct node *temp1, *temp2;

    temp1 = (struct node *)malloc(sizeof(struct node));
    temp1 -> timer = value;
    temp2 = head;

    if(head == NULL)
    {
	    /* If list is empty we create the first node. */
        head = temp1;
        head -> next = NULL;
		head -> prev = NULL;
    }

    else
    {
	    /* Traverse down to end of the list. */
        while(temp2 -> next != NULL)  
		    temp2 = temp2 -> next;  
  
        /* Append at the end of the list. */
		temp1 -> prev = temp2;
        temp1 -> next = NULL;  
        temp2 -> next = temp1;  
    }  
}

/* Deletes the first node in the list and returns 1 on success and -1 on failure */
int delBeg()
{
    if (head == NULL)
	    return -1;

    struct node *temp = head;

	if (head -> next == NULL)
	{
		free(temp);
		head = NULL;
		return 1;
	}

	/* Change second node's prev pointer to NULL and delete first node */
	else
	{
		head = temp -> next;
		head -> prev = NULL;
		free(temp);
		return 1;
	}
}

/* Deletes a node with the specified seqence number.  Returns 1 on success and -1 on failure */
int delSeqNum(uint32_t seqNum)
{
    if (head == NULL)
	    return -1;

	/* If the first node has the matching sequence number, delete it */
	if ((head -> timer).seqNum == seqNum)
	    return delBeg();

	/* Otherwise, loop until we reach the specified seqence number of the end of the list.  Delete the specified element if it has been found */
	struct node *temp;
	temp = head;
	while ((temp -> timer).seqNum != seqNum)
	{
	    temp = temp -> next;
	    if (temp == NULL)
		    return -1;
	}

	/* Rearrange the pointers of the previous and next nodes and delete the node with the specified sequence number */
	(temp -> prev) -> next = temp -> next;

	if ((temp -> next) != NULL)
	    (temp -> next) -> prev = temp -> prev;

	free(temp);
	return 1;
}

void printList()
{
    int nodeNum = 1;
    struct node *temp = head;
    while(temp != NULL)
	{
	    printf("Node %d:\n", nodeNum);
	    printf("Deltatime: %u seconds %u microseconds   PortNum: %u   SeqNum: %u\n", (temp -> timer).time.seconds, (temp -> timer).time.microSeconds, (temp -> timer).portNum, (temp -> timer).seqNum);
	    temp = temp -> next;
	    nodeNum++;
	}
}

void testDeltaTimer()
{
    struct deltaTime time1, time2, time3, time4;
    struct deltaTimer timer1, timer2, timer3, timer4;
    time1.seconds = 0;  time1.microSeconds = 200000;
    time2.seconds = 0;  time2.microSeconds = 500000;
    time3.seconds = 1;  time3.microSeconds = 300000;
	time4.seconds = 2;  time4.microSeconds = 155000;
    timer1.time = time1;  timer1.portNum = 12345;  timer1.seqNum = 305069;
    timer2.time = time2;  timer2.portNum = 12345;  timer2.seqNum = 305070;
    timer3.time = time3;  timer3.portNum = 54321;  timer3.seqNum = 305068;
	timer4.time = time4;  timer4.portNum = 98765;  timer4.seqNum = 305071;
	addBeg(timer1);
	addBeg(timer2);
	addEnd(timer3);
	addEnd(timer4);
	printList();
	printf("\nDeleting first node\n");
	delBeg();
	printList();
	printf("\nDeleting sequence num 305068\n");
	delSeqNum(305068);
	printList();
	printf("\nDeleting sequence num 305071\n");
	delSeqNum(305071);
	printList();
	printf("\nDeleting sequence num 305069\n");
	delSeqNum(305069);
	printList();
}


int main()
{
    /* Uncomment the next line to test the deltatimer */
    /* testDeltaTimer(); */
    return 0;
}
