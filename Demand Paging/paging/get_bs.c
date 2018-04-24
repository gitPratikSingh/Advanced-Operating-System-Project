#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

/*
int get_bs (bsd_t store, unsigned int npages) requests a 
new backing store with ID store of size npages (in pages, not bytes). 
If a new backing store can be created, or a backing store with
this ID already exists, the size of the new or existing backing 
store is returned."
*/
int get_bs(bsd_t bs_id, unsigned int npages) {

  /* requests a new mapping of npages with ID map_id */
    
	STATWORD ps;
    disable(ps);
	
	//kprintf("\n get_bs called for %d, %d, status %d\n", bs_id, npages, bsm_tab[bs_id].bs_status);
	
	if(isbadbs(bs_id) || isbadpagecount(npages)){
		restore(ps);
		return SYSERR;
	}
	
	// if the BS is free 
	if(bsm_tab[bs_id].bs_status == FREE){
		bsm_tab[bs_id].bs_status = UNMAPPED;
		bsm_tab[bs_id].bs_pid = currpid;
		bsm_tab[bs_id].bs_vpno = VPG_START;
		bsm_tab[bs_id].bs_npages = npages;
		bsm_tab[bs_id].next = NULL;
		//kprintf("\nReturning new npage %d\n", bsm_tab[bs_id].bs_status);
		restore(ps);
		return bsm_tab[bs_id].bs_npages;
	}else if(bsm_tab[bs_id].bs_status != HEAPED){
		// UNMAPPED/MAPPED
		//kprintf("\nReturning old npage\n");
		restore(ps);
		return bsm_tab[bs_id].bs_npages;
	}
	else{
		// private heap!
		restore(ps);
		return SYSERR;
	}	
}


