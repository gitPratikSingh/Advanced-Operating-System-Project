/* pfint.c - pfint */

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>

/*-------------------------------------------------------------------------
 * pfint - paging fault ISR
 *-------------------------------------------------------------------------
 */
SYSCALL pfint()
{
	STATWORD ps;
	disable(ps);
	
	
	//kprintf("\nPage fault start %lu", read_cr3());
	
	write_cr3(proctab[currpid].pdbr);
	
	unsigned long controlRegValue;
	controlRegValue = read_cr2();
	
	virt_addr_t *virt_addr = (virt_addr_t*)(&controlRegValue);
	pd_t *pd = (pd_t*)(proctab[currpid].pdbr); // base pagetable entry
	pd = pd + virt_addr->pd_offset; // required pagetable entry
	
	// page table not found
	if(pd->pd_pres==0){
		// page in not backed by a frame
		pt_t *new_pt = load_private_pt(pd);
		// mark the page table as present
		pd->pd_pres = 1;
		pd->pd_write = 1;
		//kprintf("\n Page table fault for process %d! for va %lu\n", currpid, controlRegValue);
	}
	
	pt_t *pt = (pt_t*)(getaddrfrompage(pd->pd_base));
	pt = pt + virt_addr->pt_offset;
	
	int ptframeidx = getframefrompage(pd->pd_base);
	frm_tab[ptframeidx].fr_refcnt++;
		
	// page not found
	if(pt->pt_pres==0){
		// page in not backed by a frame
		//kprintf("\nload_private_pg start");
		load_private_pg(pt, controlRegValue);
		//kprintf("\nload_private_pg done");
		//mark the page as present
		pt->pt_pres = 1;
		pt->pt_write = 1;
		//kprintf("\nPage fault for process %d! for va %lu\n", currpid, controlRegValue);
	}else{
		//kprintf("\nElse page fault for process %d! for va %lu\n", currpid, controlRegValue);
	}
		
	//kprintf("Page fault complete! %lu", read_cr3());
	
	restore(ps);
	return OK;
}


