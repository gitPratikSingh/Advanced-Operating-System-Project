/* chprio.c - chprio */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <stdio.h>
#include <lock.h>

/*------------------------------------------------------------------------
 * chprio  --  change the scheduling priority of a process
 *------------------------------------------------------------------------
 */
SYSCALL chprio(int pid, int newprio)
{
	STATWORD ps;    
	struct	pentry	*pptr;

	disable(ps);
	if (isbadpid(pid) || newprio<=0 ||
	    (pptr = &proctab[pid])->pstate == PRFREE) {
		restore(ps);
		return(SYSERR);
	}
	int myprevprio = getprio(pid);
	pptr->pprio = newprio;
	
	// If a process P1, in the wait queue 
	// of a lock L has its priority changed or is killed, 
	// we need to recalculate the maximum priority of all the processes 
	// in L's wait queue, and update the priority of the processes 
	// holding L too, if necessary.
	
	if(myprevprio<newprio){
	// update needed
	
		pptr->pinh = 0;
		
		if(pptr->pstate == PRLOCKED)
		{
		
			int lockidx = pptr->lockid;
			
			if(myprevprio == semlock_table[lockidx].maxwaitprio){
				// recompute maxwait again
				updatemaxwaitprio(lockidx);
			}
			
			if(myprevprio == semlock_table[lockidx].minholdprio){
				// recompute process prio again
				resetholdprocpriority(lockidx, semlock_table[lockidx].procbitmask);
			}
			
		}
		else
		{
			// PRCURR or other states 
			int i;
			for (i=0 ; i<NLOCKS ; i++) 
			{
				if(proctab[pid].lockbitmask[i] > 0){
					// recompute process hold prio again
					resetholdprocpriority(i, semlock_table[i].procbitmask);
				}
			}
			
		}
	
	}

	
	restore(ps);
	return(newprio);
}
