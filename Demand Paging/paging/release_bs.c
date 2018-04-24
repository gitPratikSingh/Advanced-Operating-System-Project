#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

SYSCALL release_bs(bsd_t bs_id) {
  /* release the backing store with ID bs_id */ 
  /*If multiple processes are mapped to the same backing store, 
  you need to unmap each mapping before calling release_bs. 
  For example 3 processes are mapped to BS1, and process 1 
  tries to release_bs, you should throw a SYSERR since the 
  BS has 3 mappings. Only after all the 3 processes have called
  unmap, should the release call be successful. Also, free_bsm 
  is more like a helper call and will not be called by the 
  process directly.*/

	STATWORD ps;
    disable(ps);
	
	if(isbadbs(bs_id)){
		restore(ps);
		return SYSERR;
	}
	
	bs_map_t *p = &bsm_tab[bs_id]; 
	bs_map_t *q = p->next;
	// multiple mappings found, retunr error!
	if(q!=NULL || p->ncount>0){
		restore(ps);
		return SYSERR;
	}
	
	p->bs_status = FREE;
	p->bs_pid = NONE;
	p->bs_vpno = VPG_START;
	p->bs_npages = NONE;
	p->bs_sem = NONE;
	
	restore(ps);
    return OK;
	
}

