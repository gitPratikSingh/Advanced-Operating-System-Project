/* bsm.c - manage the backing store mapping*/

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>
/*-------------------------------------------------------------------------
 * init_bsm- initialize bsm_tab
 {
  int bs_status;			 MAPPED or UNMAPPED		
  int bs_pid;				 process id using this slot   
  int bs_vpno;				 starting virtual page number 
  int bs_npages;			 number of pages in the store 
  int bs_sem;				 semaphore mechanism ?	
 } bs_map_t;
	
 extern bs_map_t bsm_tab[];
 *-------------------------------------------------------------------------
 *WORD *getmem(unsigned nbytes)
 */

/*Global Data Structure*/ 
int removeMap;

SYSCALL init_bsm()
{
	STATWORD ps;
    disable(ps);
	
	removeMap = 0;
	int bs;
	for(bs=0; bs<BS_COUNT; bs++){
		
		bsm_tab[bs].bs_status = FREE;
		bsm_tab[bs].bs_pid = NONE;
		bsm_tab[bs].bs_vpno = VPG_START;
		bsm_tab[bs].bs_npages = NONE;
		bsm_tab[bs].bs_sem = NONE;
		bsm_tab[bs].next = NULL;
		bsm_tab[bs].ncount = 0;
	}

	restore(ps);
    return OK;
}

/*-------------------------------------------------------------------------
 * get_bsm - get a free entry from bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL get_bsm(int* avail)
{
	STATWORD ps;
    disable(ps);
	
	int bs;
	for(bs=0; bs<BS_COUNT; bs++){
		if(bsm_tab[bs].bs_status == FREE){
			*avail = bs;
			bsm_tab[bs].bs_status = UNMAPPED;
			bsm_tab[bs].ncount =0 ;
			restore(ps);
			return OK;
		}
	}
	
	*avail = NONE;
	restore(ps);
    return SYSERR;
}


/*-------------------------------------------------------------------------
 * free_bsm - free an entry from bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL free_bsm(int i)
{
	STATWORD ps;
    disable(ps);
	
	if(isbadbs(i)){
		restore(ps);
		return SYSERR;
	}
	
	bsm_tab[i].bs_status = FREE;
	bsm_tab[i].bs_pid = NONE;
	bsm_tab[i].bs_vpno = VPG_START;
	bsm_tab[i].bs_npages = NONE;
	bsm_tab[i].bs_sem = NONE;
	bsm_tab[i].ncount = 0;
	restore(ps);
    return OK;
}

/*-------------------------------------------------------------------------
 * bsm_lookup - lookup bsm_tab and find the corresponding entry
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_lookup(int pid, long vaddr, int* store, int* pageth)
{
	STATWORD ps;
    disable(ps);
	
	if(isbadpid(pid) || isbadvaddr(vaddr)){
		restore(ps);
		return SYSERR;
	}
	
	int bs;
	int vpg = getpagefromaddr(vaddr);
	
	for(bs=0; bs<BS_COUNT; bs++){
		if(bsm_tab[bs].bs_pid == pid){
			// there can be multiple BS holding VM of a process
			int bs_vpgstr = bsm_tab[bs].bs_vpno;
			int bs_vpgend = bs_vpgstr + bsm_tab[bs].bs_npages;
			//kprintf("lookup for process %d, page: %d!,start:%d,end:%d", pid, vpg, bs_vpgstr, bs_vpgend);
			if(vpg >= bs_vpgstr && vpg < bs_vpgend){
				// match found!
				*store = bs;
				*pageth = vpg - bs_vpgstr;
				//kprintf("\nLookup match %d %d", *store, *pageth);
				// remove the mapping if called from unmap
				if(removeMap == 1)
				{
					bs_map_t *p = &bsm_tab[bs]; 
					bs_map_t *q = p->next;
					
					if(q!=NULL){
						p->bs_status = q->bs_status;
						p->bs_pid = q->bs_pid;
						p->bs_vpno = q->bs_vpno;
						p->bs_npages = q->bs_npages; 
						p->bs_sem = q->bs_sem;
						p->next = q->next;
					}
				}
				
				restore(ps);
                return OK;
			}
		}
		else
		{	// control will come here only if the BS has multiple mapps
			//might be a sub-mapping
			bs_map_t *p = &bsm_tab[bs]; 
			bs_map_t *q = p->next, *r;
			
			while(q!=NULL){
				r=p; // prev mapping
				p=q; // current mapping
				q=q->next; // next mapping 
				
				if(p->bs_pid == pid){
					int bs_vpgstr = bsm_tab[bs].bs_vpno;
					int bs_vpgend = bs_vpgstr + bsm_tab[bs].bs_npages;
					
					if(vpg >= bs_vpgstr && vpg < bs_vpgend){
						// match found!
						*store = bs;
						*pageth = vpg - bs_vpgstr;
						
						// remove the mapping if called from unmap
						if(removeMap == 1)
							r->next = q;
						
						restore(ps);
						return OK;
					}
				}
			}
		}
	}
	
	restore(ps);
    return SYSERR;
}


/*-------------------------------------------------------------------------
 * bsm_map - add an mapping into bsm_tab 
 *-------------------------------------------------------------------------
 */
 
 // called after the call to get_bsm
SYSCALL bsm_map(int pid, int vpno, int source, int npages)
{
	STATWORD ps;
    disable(ps);
	
	int vaddr = getaddrfrompage(vpno);
	
	//kprintf("\nIn bsm_map pid %d vpno %d source %d npage %d ", pid,  vpno,  source,  npages);

	if(isbadpid(pid) || isbadbs(source) || isbadvaddr(vaddr)
		// BS is private and it does not belong to the heap owner proc
		|| (bsm_tab[source].bs_status==HEAPED) ){
		//kprintf("bsmmap error!");
		restore(ps);
		return SYSERR;
	}
	
	//kprintf("\nIn bsm_map status %d", bsm_tab[source].bs_status);
	// update BS
	if(bsm_tab[source].bs_status == UNMAPPED){
		bsm_tab[source].bs_status = MAPPED;
		bsm_tab[source].bs_pid = pid;
		bsm_tab[source].bs_vpno = vpno;
		bsm_tab[source].bs_npages = npages;
		bsm_tab[source].bs_sem = NONE;
		bsm_tab[source].next = NULL;
		
	}else if(bsm_tab[source].bs_status == MAPPED){
		// map another entry
		bs_map_t *p = &bsm_tab[source]; 
		bs_map_t *q = p->next;
		
		while(q!=NULL){
			p=q;
			q=q->next; 
		}
		
		p->bs_status = MAPPED;
		p->bs_pid = pid;
		p->bs_vpno = vpno;
		p->bs_npages = npages;
		p->bs_sem = NONE;
		p->next = NULL;	
		
	}else{
		//kprintf("bsmmap error!");
		restore(ps);
		return SYSERR;
	}
	
	
	// update the process data
	proctab[currpid].vhpno=vpno;
	proctab[currpid].store=source;
	bsm_tab[source].ncount++;
	
	//kprintf("Returning OK from bsm map");
	restore(ps);
    return OK;
}



/*-------------------------------------------------------------------------
 * bsm_unmap - delete an mapping from bsm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_unmap(int pid, int vpno, int flag)
{
	STATWORD ps;
    disable(ps);
	int vaddr = getaddrfrompage(vpno);

	if(isbadpid(pid) || isbadvaddr(vaddr)){
		restore(ps);
		return SYSERR;
	}
	
	int* store; int* pageth;
	
	//kprintf("\nInside bsmunmap %d\n", bsm_tab[1].bs_status);
	
	removeMap = 1;
	int bsm_lookup_ret = bsm_lookup(pid, vaddr, store, pageth);
	removeMap = 0;
	
	if(bsm_lookup_ret == SYSERR){
		restore(ps);
		return SYSERR;
	}
	
	bsm_tab[*store].ncount--;
	
	//kprintf("\After bsmunmap %d\n", bsm_tab[1].bs_status);
	restore(ps);
    return OK;
}


