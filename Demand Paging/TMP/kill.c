/* kill.c - kill */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <mem.h>
#include <io.h>
#include <q.h>
#include <stdio.h>
#include <paging.h>

/*------------------------------------------------------------------------
 * kill  --  kill a process and remove it from the system
 *------------------------------------------------------------------------
 */
 
void removebsentries(int);
void removeProcessData(int);

SYSCALL kill(int pid)
{
	STATWORD ps;    
	struct	pentry	*pptr;		/* points to proc. table for pid*/
	int	dev;

	disable(ps);
	if (isbadpid(pid) || (pptr= &proctab[pid])->pstate==PRFREE) {
		restore(ps);
		return(SYSERR);
	}
	
	//kprintf("kill called for pid: %d",pid);
	//kprintf("\Before Kill %d\n", bsm_tab[1].ncount);
	removeProcessData(pid);
	//kprintf("\After Kill %d\n", bsm_tab[1].ncount);
	
	if (--numproc == 0)
		xdone();

	dev = pptr->pdevs[0];
	if (! isbaddev(dev) )
		close(dev);
	dev = pptr->pdevs[1];
	if (! isbaddev(dev) )
		close(dev);
	dev = pptr->ppagedev;
	if (! isbaddev(dev) )
		close(dev);
	
	send(pptr->pnxtkin, pid);

	freestk(pptr->pbase, pptr->pstklen);
	switch (pptr->pstate) {

	case PRCURR:	pptr->pstate = PRFREE;	/* suicide */
			resched();

	case PRWAIT:	semaph[pptr->psem].semcnt++;

	case PRREADY:	dequeue(pid);
			pptr->pstate = PRFREE;
			break;

	case PRSLEEP:
	case PRTRECV:	unsleep(pid);
						/* fall through	*/
	default:	pptr->pstate = PRFREE;
	}
	restore(ps);
	return(OK);
}


void removeProcessData(int pid){
/*
When a process dies, the following should happen.
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
				//kprintf("\n***********Page frame found: %d", fr);
				free_frm(fr);
			}
		}
	}
	
	removebsentries(pid);
}

void removebsentries(int pid){
	STATWORD ps;
    disable(ps);
	
	if(isbadpid(pid)){
		restore(ps);
		return SYSERR;
	}
	
	int bs;
	
	for(bs=0; bs<BS_COUNT; bs++){
		if(bsm_tab[bs].bs_status!=FREE)
		{	
		if(bsm_tab[bs].bs_pid == pid){
			bs_map_t *p = &bsm_tab[bs]; 
			bs_map_t *q = p->next;
			
			if(q!=NULL){
				p->bs_status = q->bs_status;
				p->bs_pid = q->bs_pid;
				p->bs_vpno = q->bs_vpno;
				p->bs_npages = q->bs_npages; 
				p->bs_sem = q->bs_sem;
				p->next = q->next;
			}else{
				// free the BSM
				if(bsm_tab[bs].ncount == 1){
					free_bsm(bs);
				}else{
					bsm_tab[bs].ncount--;
				}
			}
		
			restore(ps);
			return OK;
		}
		else
		{	// control will come here only if the BS has multiple mapps
			//might be a sub-mapping
			bs_map_t *p = &bsm_tab[bs]; 
			bs_map_t *q = p->next, *r;
			
			//kprintf("\n***Else");
			while(q!=NULL){
				r=p; // prev mapping
				p=q; // current mapping
				q=q->next; // next mapping 
				
				if(p->bs_pid == pid){
					r->next = q;
					//kprintf("\n***Else %d", pid);
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