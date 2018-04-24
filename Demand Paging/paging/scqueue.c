#include <paging.h>

scitem scadd(scitem *scqhead, int frno)
{
	scitem *temp = scqhead;
	
	while(temp->next != NULL){
		temp = temp->next;
	}
	
	scitem nextItem;
	nextItem.frno = frno;
	nextItem.next = NULL;
	
	temp->next = nextItem;
	
	return(nextItem);
}


scitem scdelete(scitem *scqhead, int frno)
{
	scitem *temp = scqhead;
	scitem *prev = NULL;
	
	while(temp->next != NULL){
		if(temp->frno == frno)
			break;
		prev = temp;
		temp = temp->next;
	}
	
	if(temp->frno == frno){
		// case 1, if first item is to be deleted, reset the scqhead
		if(temp == scqhead){
			scqhead = temp->next;
			return;
		}
		
		// case 2, if last item is to be deleted
		if(temp->next == NULL){
			prev->next = NULL;
			return;
		}
		
		// case 3: in between
		prev->next = temp->next;
		return;
		
	}else{
		//kprintf("SC Error, frame not found while dequeuing");
	}	
}