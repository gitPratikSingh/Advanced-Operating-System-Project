/* xm.c = xmmap xmunmap */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>


/*-------------------------------------------------------------------------
 * xmmap - xmmap
 *-------------------------------------------------------------------------
 */
SYSCALL xmmap(int virtpage, bsd_t source, int npages)
{
  
	STATWORD ps;
    disable(ps);
	
	//kprintf("\nIn xmmap for pid: %d vp %d sr %d  npage %d ", currpid, virtpage, source, npages);
	//kprintf("isbadbs %d isbadpagecount %d  isbadvaddr %d ", isbadbs(source) , isbadpagecount(npages) , isbadvaddr(getaddrfrompage(virtpage)));
	
	if(isbadbs(source) 
		|| isbadpagecount(npages) 
		|| isbadvaddr(getaddrfrompage(virtpage))){
		restore(ps);
		return SYSERR;
	}
	
    if (bsm_tab[source].bs_status == FREE 
		|| bsm_tab[source].bs_status == HEAPED
		|| bsm_tab[source].bs_npages < npages) {
        restore(ps);
		return SYSERR;
    }

	// kprintf("\nNo arg error in xmap\n");
	// same address range should not be mapped to more than one BS
	int store;
	int pageOffset;
	
	int bsm_lookup_ret = bsm_lookup(currpid, getaddrfrompage(virtpage), &store, &pageOffset);
	
	if(bsm_lookup_ret == OK){
		restore(ps);
		return SYSERR;
	}
	
	//kprintf("\nNo arg error in xmap.bsm_lookup\n");
	
    /* the same BS should not be mapped for more than one mapping for a process. */
	int hasPrevMap =0;
	bs_map_t *temp=&bsm_tab[source];
	
	while(temp!=NULL){
		//kprintf("\nInside loop");

		if(temp->bs_status == MAPPED && temp->bs_pid == currpid){
			hasPrevMap = 1;
			break;
		}
		temp = temp->next;
	}
	
	//kprintf("\nOutside loop");
	
	if(hasPrevMap){
		restore(ps);
		return SYSERR;
	}
	
	// every thing looks set, map the range
	int bsm_map_ret = bsm_map(currpid, virtpage, source, npages);
	// kprintf("\nbsm_map_ret %d SYSERR %d", bsm_map_ret, SYSERR);
	
	if(bsm_map_ret == SYSERR){
		restore(ps);
		return SYSERR;
	}else{
		restore(ps);
		return OK;
	}
}



/*-------------------------------------------------------------------------
 * xmunmap - xmunmap
 *-------------------------------------------------------------------------
 */
SYSCALL xmunmap(int virtpage)
{

	STATWORD ps;
	disable(ps);

	//kprintf("\nXmunmap Init");
	
	if(isbadvaddr(getaddrfrompage(virtpage))){
		restore(ps);
		return SYSERR;
	}
	//kprintf("\nXmunmap Step 1");
	
	int bs_id;
	int pageth;
	int bsm_lookup_ret = bsm_lookup(currpid, getaddrfrompage(virtpage), &bs_id, &pageth);	
	
	if (bsm_lookup_ret == SYSERR
		|| bsm_tab[bs_id].bs_status == FREE 
		|| bsm_tab[bs_id].bs_status == HEAPED) {
        restore(ps);
		return SYSERR;
    }
	
	//kprintf("\nXmunmap Intermediate");
	
	int bsm_unmap_ret = bsm_unmap(currpid, virtpage, TRUE);
	if(bsm_unmap_ret == SYSERR){
		restore(ps);
		return SYSERR;
	}
	
	//kprintf("\nXmunmap finished");
	
	restore(ps);
	return OK;
	
}
