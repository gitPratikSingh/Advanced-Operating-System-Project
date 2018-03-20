/* kill.c - kill */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <mem.h>
#include <io.h>
#include <q.h>
#include <stdio.h>
#include <lock.h>
/*------------------------------------------------------------------------
 * kill  --  kill a process and remove it from the system
 *------------------------------------------------------------------------
 */
 
void releaselock(int, int);

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
		
	int myprio = getprio(pid);
	
	
	// if a process is killed, release its locks if any
	// update the lock semantics nr,nw,state etc
	int i;
	for (i=0 ; i<NLOCKS ; i++) 
	{
		if(proctab[pid].lockbitmask[i] > 0){
			releaselock(i, pid);
		}
	}
	
	if(pptr->pstate == PRLOCKED){
		// If a process P1, in the wait queue 
		// of a lock L has its priority changed or is killed, 
		// we need to recalculate the maximum priority of all the processes 
		// in L's wait queue, and update the priority of the processes 
		// holding L too, if necessary.
		int lockidx = pptr->lockid;
		pptr->lockid = -1;
		
		if(myprio == semlock_table[lockidx].maxwaitprio){
			// recompute maxwait again
			updatemaxwaitprio(lockidx);
			//kprintf("\nmax wait prio %d\n", semlock_table[lockidx].maxwaitprio);
		}
		
		if(myprio == semlock_table[lockidx].minholdprio){
			// recompute process prio again
			resetholdprocpriority(lockidx, semlock_table[lockidx].procbitmask);
			//kprintf("\n minholdprio %d\n", semlock_table[lockidx].minholdprio);
		}
		
		// remove the process from the waiting queue
		dequeue(pid);
		pptr->pstate = PRFREE;
		
	}
	
	// clean up the lock semantics
	for (i=0 ; i<NLOCKS ; i++) 
	{
		proctab[pid].lockbitmask[i] = 0;
	}
	
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


void resetholdprocpriority(int lockidx, int procbitmask[])
{	
	// updates the priority of the holding processes of this lock

	// kprintf("\n\n Inside resetholdprocpriority \n\n");
	int j;
	int min=MAXINT;
	for(j=0; j<NPROC; j++)
	{	// update the prio of the procs holding the lock
		if(procbitmask[j] == 1)
		{
			int prev = getprio(j);
			// reset the priorities
			updatemyprio(j);
			
			// kprintf("\nprev prio: %d, cur prio: %d, procIdx %d", prev, getprio(j), j);
			
			if(min>getprio(j)){
				min = getprio(j);
			}
			// ensure transitivity of prio update 
			// i.e if this process is waiting for a lock some other procs update their prio
			if(prev != getprio(j) && proctab[j].pstate == PRLOCKED && proctab[j].lockid != -1){
				int lockid = proctab[j].lockid;
				// if the next lock's maxwaitprio was set due to this process's previous prio, reset it
				if(prev == semlock_table[lockid].maxwaitprio){
					updatemaxwaitprio(lockid);
				}
				
				resetholdprocpriority(lockid, semlock_table[lockid].procbitmask);
			}
		}
	}
	
	if(min == MAXINT){
		min = -1;
	}
	
	semlock_table[lockidx].minholdprio = min;	
}


void releaselock(int lock, int pid)
{
	register struct	semlock	*sptr;	
	struct	pentry	*pptr;
	pptr = &proctab[pid];
	pptr->waitStart = -1;
	
	int semlock_index;
	int hasError = 0;
	int myprio = getprio(pid);

	semlock_index = lock;
	// check validity of the lock
	// i) bad semlock_index
	// ii) semlock is free
	// iii) proc does not hold semlock 
	if (isbadsemlock(semlock_index) 
		|| (sptr= &semlock_table[semlock_index])->sstate==LFREE 
		|| pptr->lockbitmask[semlock_index] <= 0) 
	{
		hasError = 1;
	}
	
	// update some vars associated with this lock
	pptr->lockbitmask[semlock_index] = 0;
	sptr->procbitmask[pid] = 0;
	
	if(sptr->minholdprio==myprio){
		updateminholdprio(semlock_index);
	}
	
	// current process is releasing the lock, hence decrease count
	if(sptr->type_of_locking==READ){
		sptr->nr--;
	}else if(sptr->type_of_locking==WRITE){
		sptr->nw--;
	}
			
	// Case 1: No waiting writers and no readers
	if(isempty(sptr->sqhead_r)&& isempty(sptr->sqhead_w))
	{
		// no active user of the lock
		// reset the lock
		if(sptr->nr==0 && sptr->nw==0){
			sptr->sstate = LFREE;
			sptr->type_of_locking = NOLOCK;
			sptr->minholdprio = -1;
			sptr->maxwaitprio = -1;
		}
	}
	else
	{
		// now pick the proc with highest lock priority to run
		int r_max = lastkey(sptr->sqtail_r);
		int w_max = lastkey(sptr->sqtail_w);
		
			// if there is no active reader, then writer/reader can get in if it has max priority
			if(sptr->nr==0){
				if(r_max>w_max)
				{
					// reader is selected
					RemoveReadersFromQueue(sptr->sqhead_r, sptr->sqtail_r, r_max, w_max, -1);
					sptr->type_of_locking=READ;
				}
				else if(r_max==w_max)
				{
						// If a reader's lock priority is equal to the highest lock priority of 
						// the waiting writer and its waiting time is no more than 0.5 second longer, 
						// the reader should be given preference to acquire the lock over the 
						// waiting writer
						// this case can occur if there are both Rs and Ws
						
						int r_max_proc_id = lastid(sptr->sqtail_r), w_max_proc_id = lastid(sptr->sqtail_w);
						struct	pentry	*pptr = &proctab[r_max_proc_id], *pptw = &proctab[w_max_proc_id];
						
						int r_waittime = (ctr1000 - pptr->waitStart);
						int w_waittime = (ctr1000 - pptw->waitStart);
						
						
						if(r_waittime>=w_waittime || (w_waittime - r_waittime)<=500)
						{
							// allow reader
							RemoveReadersFromQueue(sptr->sqhead_r, sptr->sqtail_r, r_max, w_max, pptw->waitStart);
							sptr->type_of_locking=READ;
						}
						else
						{
							// allow writer
							RemoveWriterFromQueue(sptr->sqhead_w,sptr->sqtail_w);
							sptr->type_of_locking=WRITE;
						}
							
				}	
				else
				{	// writer has the max priority
						RemoveWriterFromQueue(sptr->sqhead_w, sptr->sqtail_w);
						sptr->type_of_locking=WRITE;
				}
					
			}
			else
			{
				// there is already an active reader, all waiting reader with priority >= max waiting writer can go in
				RemoveReadersFromQueue(sptr->sqhead_r, sptr->sqtail_r, r_max, w_max, -1);
				sptr->type_of_locking=READ;
			}	
	}

}
