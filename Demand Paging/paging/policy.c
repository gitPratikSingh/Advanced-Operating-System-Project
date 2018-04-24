/* policy.c = srpolicy*/

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <q.h>

int debug = 0;
extern int page_replace_policy;
scitem* scqhead;

// Second-Chance (SC) or Least-Frequently-Used (LFU). 
// You can declare constant SC as 3 and LFU as 4 

int initpolicy(int policy){

	if(policy != SC && policy != LFU){
		return -1;
	}
	
	scqhead = NULL;
	
	return 0;
}

/*-------------------------------------------------------------------------
 * srpolicy - set page replace policy 
 *-------------------------------------------------------------------------
 */
SYSCALL srpolicy(int policy)
{
  /* sanity check ! */

  if(policy != SC && policy != LFU){
	return SYSERR;
  }
	
  // init policy datastructure
  initpolicy(policy);
  
  page_replace_policy = policy;

  //kprintf("\nPolicy set!");
  
  debug = 1;
  return OK;
}

/*-------------------------------------------------------------------------
 * grpolicy - get page replace policy 
 *-------------------------------------------------------------------------
 */
SYSCALL grpolicy()
{
  return page_replace_policy;
}

/* implementation of circular queue*/

void scadd(int frno)
{
	scitem *temp = scqhead;
	
	scitem *nextItem = (scitem*)getmem(sizeof(scitem));
	nextItem->frno = frno;
	nextItem->next = NULL;
	nextItem->prev = NULL;
	
	if(scqhead == NULL){
		scqhead = nextItem;
		nextItem->next = scqhead;
		nextItem->prev = scqhead;
		return;
	}
	
	nextItem->prev = scqhead->prev;
	nextItem->next = scqhead;
	
	scqhead->prev->next = nextItem;
	scqhead->prev = nextItem;
	
	//kprintf("\n Inside add\n");
	//kprintf("scqhead->fr; %d\n", scqhead->frno);
	
	return;
}


void scdelete(int frno)
{
	scitem *temp = scqhead;
	scitem *prev = NULL;
	
	if(scqhead == NULL)
		return;
	
	while(1){
		if(temp->frno == frno){
			
			temp->prev->next = temp->next;
			temp->next->prev = temp->prev;
			
			// if item deleted is the head, adjust the head pointer
			if(temp == scqhead){
				scqhead = scqhead->next;
			}
			
			return;
		}else{
			
			temp = temp->next;
		}
		
		
		if(temp == scqhead){
			return;
		}
	}
}