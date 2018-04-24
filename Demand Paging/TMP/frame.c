/* frame.c - manage physical frames */
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

/*
typedef struct{
  int fr_status;			MAPPED or UNMAPPED		
  int fr_pid;				 process id using this frame  
  int fr_vpno;				 corresponding virtual page no
  int fr_refcnt;			 reference count		
  int fr_type;				 FR_DIR, FR_TBL, FR_PAGE	
  int fr_dirty;
}fr_map_t;
*/

fr_map_t frm_tab[NFRAMES];

/*-------------------------------------------------------------------------
 * init_frm - initialize frm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL init_frm()
{
	STATWORD ps;
    disable(ps);
	
	//note FRAME0 = page 1024
	int fr;
	for(fr=0; fr<NFRAMES; fr++){
		frm_tab[fr].fr_status = UNMAPPED;
		frm_tab[fr].fr_pid = NONE;
		frm_tab[fr].fr_vpno = NONE;
		frm_tab[fr].fr_refcnt = 0;
		frm_tab[fr].fr_type = NONE;
		frm_tab[fr].fr_dirty = 0;
		frm_tab[fr].fr_use_counter = 0;
	}
	
	restore(ps);
	return OK;
}


/*-------------------------------------------------------------------------
 * get_frm - get a free frame according page replacement policy
 *-------------------------------------------------------------------------
 */
SYSCALL get_frm(int* avail)
{
  /* Find a free frame.
     * IF not free frame then, apply page replacement. 
	 * Note - pt and pd frames can be range of 1024 through 1535. 
	 * 0-1023 = Kernel!
     */
	 
	STATWORD ps;
    disable(ps);
	
	int fr;
	for(fr=0; fr<NFRAMES; fr++){
		if(frm_tab[fr].fr_status == UNMAPPED){
			frm_tab[fr].fr_type = FR_PAGE;
			frm_tab[fr].fr_status = MAPPED;
			frm_tab[fr].fr_pid = currpid;
			
			*avail = fr;
			
			//kprintf("\n Found Free Frame %d", fr);
			scadd(fr);
			
			restore(ps);
			return OK;
		}
	}
	
	// no free frames, hence apply pg rep algo
	
	//kprintf("\n No Found Free Frame!,printing the SC queue \n");
	/*scitem *temp = scqhead;
	
	for(fr=0; fr<NFRAMES; fr++){
		
		if(frm_tab[temp->frno].fr_type == FR_PAGE)
			kprintf(" Page(%d) ->", temp->frno);
		if(frm_tab[temp->frno].fr_type == FR_TBL)
			kprintf("table(%d) ->", temp->frno);
		if(frm_tab[temp->frno].fr_type == FR_DIR)
			kprintf("dir(%d) ->", temp->frno);
		
		
		temp = temp->next;
		if(temp==scqhead){
			kprintf("\n");
			break;
		}
	}
	*/
	
	int victimFrm = getVictimFrame(grpolicy());
	if(debug == 1){
		kprintf("Replaced frame %d \n", victimFrm);
	}
	
	free_frm(victimFrm);
	
	// for FR_TBL, FR_DIR, they will be updated later after this call finishes
	frm_tab[victimFrm].fr_type = FR_PAGE;
	frm_tab[victimFrm].fr_status = MAPPED;
	frm_tab[victimFrm].fr_pid = currpid;
	frm_tab[victimFrm].fr_use_counter++;
	
	*avail = victimFrm;
	scadd(victimFrm);
	
	restore(ps);
	return OK;
}

int getVictimFrame(int policy){
	// compute the victim frame that will be evicted/removed 
	// and new page will be placed on it
	// read_cr3();
	// write_cr3(proctab[currpid].pdbr);
	
	if(policy == SC){
		
		//no need to support paging of FR_TBL, FR_DIR 
		// start reading the queue	
		
		if(scqhead == NULL){
			kprintf("SC Queue head is not initialized");
			return -1;
		}
		
		while(1){
			int fr = scqhead->frno;
			// check the pt flag of this page
			int pid = frm_tab[fr].fr_pid;
			
			if(frm_tab[fr].fr_type != FR_PAGE){
				scqhead = scqhead->next;
				continue;
			}

			unsigned long addr = frm_tab[fr].fr_vpno*NBPG;
			virt_addr_t *virt_addr = (virt_addr_t*)(&addr);
			
			pd_t *pd = (pd_t*)(proctab[pid].pdbr);
			pd = pd + virt_addr->pd_offset; 
			
			pt_t *pt = (pt_t*)(getaddrfrompage(pd->pd_base));
			pt = pt + virt_addr->pt_offset;

			scqhead = scqhead->next;
			
			kprintf("\nPt_access flag %d page offset %d\n", pt->pt_acc, virt_addr->pt_offset);
			
			if(pt->pt_acc == 1){
				pt->pt_acc = 0;
			}else{
				return fr;
			}
		}
	}
	else
	{
		int fr;
		int minref = MAXINT;
		int minreffr = NONE;
		
		for(fr=0; fr<NFRAMES; fr++){
			// not needed : support paging of FR_TBL, FR_DIR 
			if(frm_tab[fr].fr_type!=FR_PAGE)
				continue;
				
			if(frm_tab[fr].fr_use_counter<minref){
				minref = frm_tab[fr].fr_use_counter;
				minreffr = fr;
			}else if(frm_tab[fr].fr_use_counter==minref){
				// compare the vpno if ref counts are same
				if(frm_tab[fr].fr_vpno>frm_tab[minref].fr_vpno){
					minreffr = fr;
				}
			}
		}
	}
	
	return 0;
}
/*-------------------------------------------------------------------------
 * free_frm - free a frame 
 *-------------------------------------------------------------------------
 */
SYSCALL free_frm(int i)
{
 STATWORD ps;
 disable(ps);
	
	if(isbadframe(i)){
		restore(ps);
		return SYSERR;
	}
	
	kprintf("Free frame called for fr: %d", i);
	
	// update the sc queue
	// safe check is present in the function
	scdelete(i);
	
	int pid = frm_tab[i].fr_pid;
	int bs_id = proctab[pid].store;
	int proc_start_fr_vpno = proctab[pid].vhpno;
	int fr_vpno = frm_tab[i].fr_vpno;
	int pageOffset = fr_vpno - proc_start_fr_vpno;
	
	char* frmAddr = (char*)getaddrfrompage(getpagefromframe(i));
	// copy the frame back to BS
	write_bs(frmAddr, bs_id, pageOffset);
	
	if(pid == currpid){
		// invalidate the TLB cache
		// invlpg 
		read_cr3();
		write_cr3(proctab[currpid].pdbr);
	}
	
	
	// now update the pt_t	
	unsigned long addr = fr_vpno*NBPG;
	virt_addr_t *virt_addr = (virt_addr_t*)(&addr);
	pd_t *pd = (pd_t*)(proctab[pid].pdbr);
	pd = pd + virt_addr->pd_offset; 

	pt_t *pt = (pt_t*)(getaddrfrompage(pd->pd_base));
	pt = pt + virt_addr->pt_offset;

	int ptframeidx = getframefrompage(pd->pd_base);
	frm_tab[ptframeidx].fr_refcnt--;
	
	if(frm_tab[ptframeidx].fr_refcnt == 0){
		
		pd->pd_pres = 0;
		
		frm_tab[ptframeidx].fr_status = UNMAPPED;
		frm_tab[ptframeidx].fr_pid = NONE;
		frm_tab[ptframeidx].fr_vpno = NONE;
		frm_tab[ptframeidx].fr_refcnt = 0;
		frm_tab[ptframeidx].fr_type = NONE;
		frm_tab[ptframeidx].fr_dirty = 0;
	}
	
	if(frm_tab[i].fr_type==FR_PAGE)
		pt->pt_pres = 0;
	else if(frm_tab[i].fr_type==FR_TBL)
		pd->pd_pres = 0;
	
	frm_tab[i].fr_status = UNMAPPED;
	frm_tab[i].fr_pid = NONE;
	frm_tab[i].fr_vpno = NONE;
	frm_tab[i].fr_type = NONE;
	frm_tab[i].fr_dirty = 0;	

 restore(ps);
 return OK;
}


