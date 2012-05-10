/* Implementation of delta timer using a doubly linked list.
   Code for inserting, deleting, and traversing nodes was taken from
   http://www.snippets.24bytes.com/2010/06/double-linked-list.html
*/

#include <stdio.h>

typedef struct deltaTimer
{
	float deltaTime;
	unsigned portNum;
	unsigned seqNum;
} deltaTimer;

class node
{
    public:
    deltaTimer value;    //value stored in the node
    node *next;          //pointer to next node
    node *prev;          //pointer to previous node
};

class dlist
{
    public:
        node *front;       //pointer to front of list   
        node *back;        //pointer to back of list  

	dlist()
	{
		front=NULL;
		back=NULL;
	}

	void insertFront(deltaTimer value);             
	void insertBack(deltaTimer value);
	void removeFront();
	void removeBack();
	void insertBefore(deltaTimer value,node *nodeB);
	void insertAfter(deltaTimer value,node *nodeA);
	void removeBefore(node *nodeB);
	void removeAfter(node *nodeA);
	void removeNode(node *newNode);
	void printDListFront();
	void printDListBack();
};

//insert a node before nodeB
void dlist::insertBefore(deltaTimer value,node *nodeB)    
{
	node *newNode;
	newNode=new node();
	newNode->prev=nodeB->prev;
	newNode->next =nodeB;
	newNode->value =value; 
	if(nodeB->prev==NULL)
	{
		this->front=newNode; 
	}
	nodeB->prev=newNode;
}

//insert a node before the front node 
void dlist::insertFront (deltaTimer value)
{
	node *newNode;
	if(this->front==NULL)
	{
		newNode=new node();
		this->front=newNode;
		this->back =newNode;
		newNode->prev=NULL;
		newNode->next=NULL;
		newNode->value=value;
	}
	
	else
	{
		insertBefore(value,this->front );
	}
}

//insert a node after  nodeB
void dlist::insertAfter(deltaTimer value, node *nodeB)
{
	node *newNode;
	newNode=new node();
	newNode->next= nodeB->next ;
	newNode->prev  =nodeB;
	newNode->value =value;

	if(nodeB->next==NULL)
		this->back =newNode;
	
	nodeB->next=newNode;
}

//insert a node after the last node 
void dlist::insertBack (deltaTimer value)
{          
	if(this->back==NULL)
		insertFront(value);
	
	else
		insertAfter(value,this->back  );
}

//remove the front node 
void dlist::removeFront ()
{
	removeNode(this->front);
}

//remove a back node 
void dlist::removeBack  ()
{
	removeNode(this->back);
}

//remove before a node 
void dlist::removeBefore(node *nodeB)
{
	if(nodeB->prev==this->front)
	{
		this->front=nodeB;
		this->front->prev=NULL;
	}
	else
	{
		removeNode(nodeB->prev);
	}
}

//remove after a node 
void dlist::removeAfter(node *nodeA)
{
	if(nodeA->next==this->back)
	{
		this->back=nodeA;
		this->back->next=NULL;
	}
	else
	{
		removeNode(nodeA->next);
	}
}

//remove a perticular node 
void dlist::removeNode(node *nodeToRemove)
{
	if(nodeToRemove==this->front)
	{
		this->front=this->front->next;
		this->front->prev=NULL;
	}
	
	else if (nodeToRemove==this->back)
	{
		this->back=this->back->prev;
		this->back->next=NULL ;
	}
	
	else
	{
		nodeToRemove->prev->next=nodeToRemove->next;
		nodeToRemove->next->prev=nodeToRemove->prev;
	}
}

//Print the list from front 
void dlist::printDListFront()
{
	node* curr2;
	curr2= this->front;
	printf("printDListFront:\n");
	
	while(curr2!=NULL)
	{
		printf("DeltaTime: %f PortNum: %u SeqNum: %u\n", curr2->value.deltaTime, curr2->value.portNum, curr2->value.seqNum);
		curr2=curr2->next;
	}

	printf("\n");

}


//Print the Double Linked List from backwards
void dlist::printDListBack()
{
	node* curr2;
	curr2= this->back;
	printf("printDListBack:\n");
	
	while(curr2!=NULL)
	{
		printf("DeltaTime: %f PortNum: %u SeqNum: %u\n", curr2->value.deltaTime, curr2->value.portNum, curr2->value.seqNum);
		curr2=curr2->prev;
	}
	printf("\n");
}

int main()
{
	deltaTimer dt;
	dt.deltaTime = .1;
	dt.portNum = 30701;
	dt.seqNum = 34329872;
	
	deltaTimer dt1;
	dt1.deltaTime = .2;
	dt1.portNum = 30701;
	dt1.seqNum = 34329873;
	
	dlist *st ;
	st= new dlist();
	st->insertBack(dt); 
	st->printDListFront();
	st->insertBack(dt1); 
	st->printDListBack();
	st->removeFront();
	st->printDListFront();
	st->removeFront();
	return 0;
}
