#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>
#include <paging.h>

pd_t* load_pd(int);
pt_t* load_private_pt(pd_t*);
SYSCALL load_private_pg(pt_t*, unsigned long);
void WriteBackProcessData(int);
/*
typedef struct {

  unsigned int pt_pres	: 1;		 page is present?		
  unsigned int pt_write : 1;		 page is writable?		
  unsigned int pt_user	: 1;		 is use level protection?	
  unsigned int pt_pwt	: 1;		 write through for this page? 
  unsigned int pt_pcd	: 1;		 cache disable for this page? 
  unsigned int pt_acc	: 1;		 page was accessed?		
  unsigned int pt_dirty : 1;		 page was written?		
  unsigned int pt_mbz	: 1;		 must be zero			
  unsigned int pt_global: 1;		 should be zero in 586	
  unsigned int pt_avail : 3;		 for programmer's use		
  unsigned int pt_base	: 20;		 location of page?		
} pt_t/pd_t;

*/

pd_t* load_pd(int pid){
	STATWORD ps;
    disable(ps);
	
	int setReg = (pid == NULLPROC);
	
	int newfrmidx;
	if(SYSERR == get_frm(&newfrmidx))
	{
		restore(ps);	
		return NULL;
	}
	
	frm_tab[newfrmidx].fr_type = FR_DIR;
	frm_tab[newfrmidx].fr_status = MAPPED;
	frm_tab[newfrmidx].fr_pid = pid;
	
	
	// get the page to which this frame is mapped
	int pgno = getpagefromframe(newfrmidx);
	pd_t *new_pd = (pd_t*)getaddrfrompage(pgno);
	
	//kprintf("\nInside pid %d create_pd: newfrmidx %d", pid, newfrmidx);
	
	int pd_idx;
	for (pd_idx = 0; pd_idx < MAX_PT_PER_PD; pd_idx++) {
        new_pd[pd_idx].pd_pres = pd_idx<SHARED_PGT_COUNT?1:0;
        new_pd[pd_idx].pd_write = 1;
        new_pd[pd_idx].pd_user = 0;
        new_pd[pd_idx].pd_pwt = 0;
        new_pd[pd_idx].pd_pcd = 0;
        new_pd[pd_idx].pd_acc = 0;
        new_pd[pd_idx].pd_fmb = 0;
        new_pd[pd_idx].pd_mbz = 0;
        new_pd[pd_idx].pd_global = 0;
        new_pd[pd_idx].pd_avail = 0;
		// location of the page table 
        new_pd[pd_idx].pd_base = pd_idx+FRAME0;
    }
	// set the page dir 
	proctab[pid].pdbr = getaddrfrompage(pgno);
	
	// Set the PDBR register to the page directory
	if(setReg == 1)
		write_cr3(proctab[pid].pdbr);
	
	restore(ps);
	return new_pd;
}

pt_t* load_shared_pt(int ptIdx){
	// create a new page table
	STATWORD ps;
    disable(ps);
	
	int newfrmidx;
	if(SYSERR == get_frm(&newfrmidx))
	{
		restore(ps);	
		return NULL;
	}
	
	frm_tab[newfrmidx].fr_type = FR_TBL;
	frm_tab[newfrmidx].fr_status = MAPPED;
	frm_tab[newfrmidx].fr_pid = currpid;

	// get the page to which this frame is mapped
	int pgno = getpagefromframe(newfrmidx);
	pt_t *new_pt = (pt_t*)getaddrfrompage(pgno);
	
	int page_idx;
	for (page_idx = 0; page_idx < MAX_PG_PER_PT; page_idx++) {
        new_pt[page_idx].pt_pres = 1;
        new_pt[page_idx].pt_write = 1;
        new_pt[page_idx].pt_user = 0;
        new_pt[page_idx].pt_pwt = 0;
        new_pt[page_idx].pt_pcd = 0;
        new_pt[page_idx].pt_acc = 0;
        new_pt[page_idx].pt_dirty = 0;
        new_pt[page_idx].pt_mbz = 0;
        new_pt[page_idx].pt_global = 1;
        new_pt[page_idx].pt_avail = 0;
		
		// store the address of the page in pt_base
		// need to be redefined for virtual pages by the frame of the corresponding page! 
        new_pt[page_idx].pt_base = (MAX_PG_PER_PT * ptIdx) + page_idx;
    }
		
	restore(ps);
	return new_pt;
}

pt_t* load_private_pt(pd_t *pd){
	// create a new page table
	STATWORD ps;
    disable(ps);
	
	int newfrmidx;
	if(SYSERR == get_frm(&newfrmidx))
	{
		restore(ps);	
		return NULL;
	}
	
	frm_tab[newfrmidx].fr_status = MAPPED;
	frm_tab[newfrmidx].fr_pid = currpid;
	frm_tab[newfrmidx].fr_vpno = 0; //vpg no does not makes sense for page table, vpgno are mapped to pages only
	frm_tab[newfrmidx].fr_refcnt++;
	frm_tab[newfrmidx].fr_type = FR_TBL;
	frm_tab[newfrmidx].fr_dirty = 0;

	pd->pd_pres = 1;
	pd->pd_write = 1;
	pd->pd_base = (FRAME0 + newfrmidx);
	
	// get the page to which this frame is mapped
	int pgno = getpagefromframe(newfrmidx);
	pt_t *new_pt = (pt_t*)getaddrfrompage(pgno);
	
	int page_idx;
	for (page_idx = 0; page_idx < MAX_PG_PER_PT; page_idx++) {
        new_pt[page_idx].pt_pres = page_idx == 27?1:0;
        new_pt[page_idx].pt_write = 0;
        new_pt[page_idx].pt_user = 0;
        new_pt[page_idx].pt_pwt = 0;
        new_pt[page_idx].pt_pcd = 0;
        new_pt[page_idx].pt_acc = 0;
        new_pt[page_idx].pt_dirty = 0;
        new_pt[page_idx].pt_mbz = 0;
        new_pt[page_idx].pt_global = 0;
        new_pt[page_idx].pt_avail = 0;
		new_pt[page_idx].pt_base = 0;
    }
	
	//kprintf("\nPage table created on fr %d", newfrmidx);
	
	restore(ps);
	return new_pt;
}

SYSCALL load_private_pg(pt_t *pt, unsigned long vaddr){
	// create a new page table
	STATWORD ps;
    disable(ps);
	
	int newfrmidx;
	if(SYSERR == get_frm(&newfrmidx))
	{
		restore(ps);	
		return NULL;
	}
	
	frm_tab[newfrmidx].fr_status = MAPPED;
	frm_tab[newfrmidx].fr_pid = currpid;
	frm_tab[newfrmidx].fr_vpno = getpagefromaddr(vaddr);
	frm_tab[newfrmidx].fr_refcnt++;
	frm_tab[newfrmidx].fr_type = FR_PAGE;
	frm_tab[newfrmidx].fr_dirty = 0;

	pt->pt_pres = 1;
	pt->pt_write = 1;
	pt->pt_base = (FRAME0 + newfrmidx);
	
	//check if this page exists in BS
	int store, offset;
	int ret = bsm_lookup(currpid, vaddr, &store, &offset);
	if(ret == OK){
		//kprintf("\nReading back from store for proc %d!, store:%d, offset:%d ",currpid, store, offset);
		read_bs((char*)((pt->pt_base)*NBPG), store, offset);
	}else{
		kprintf("\nInvalid address, process will be killed now");
		restore(ps);
		kill(currpid);
	}
	
	//kprintf("\nPage created on fr %d", newfrmidx);
	
	restore(ps);
	return OK;
}

int init_shared_pt(){
	
	int spt =0;
	for(spt=0; spt<SHARED_PGT_COUNT; spt++){
		load_shared_pt(spt);
	}
	
	return OK;
}


virt_addr_t* mapToVirtAddr(unsigned long n){
	virt_addr_t *virtAddr;
	virtAddr = (virt_addr_t*)(&n);
	return virtAddr;
}


void WriteBackProcessData(int pid){
/*
When a process dies/sleeps, the following should happen.
	1. All frames which currently hold any of its pages should be written 
		to the backing store and be freed.
	2. All of its mappings should be removed from the backing store map.
	3. The backing stores for its heap (and stack if have chosen to 
		implement a private stack) should be released (remember backing 
		stores allocated to a process should persist unless the process 
		explicitly releases them).
	4. The frame used for the page directory should be released.
*/

	int fr;
	for(fr=0; fr<NFRAMES; fr++){
		if(frm_tab[fr].fr_pid == pid){
			if(frm_tab[fr].fr_type == FR_PAGE)
			{	
				int bs_id = proctab[pid].store;
				int proc_start_fr_vpno = proctab[pid].vhpno;
				int fr_vpno = frm_tab[fr].fr_vpno;
				int pageOffset = fr_vpno - proc_start_fr_vpno;
				
				char* frmAddr = (char*)getaddrfrompage(getpagefromframe(fr));
				// copy the frame back to BS
				write_bs(frmAddr, bs_id, pageOffset);
			}
		}
	}
}
