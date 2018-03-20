/* signal.c - signal */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <lock.h>
#include <stdio.h>

/*------------------------------------------------------------------------
 * signal  --  signal a semaphore, releasing one waiting process
 *------------------------------------------------------------------------
 * process which is calling this function will release the lock
 */
SYSCALL releaseall(numlocks, ldes1)
int numlocks;
int ldes1; /* arguments (treated like an array in the code)*/
{
	STATWORD ps;    
	register struct	semlock	*sptr;

	disable(ps);
	
	struct	pentry	*pptr;
	pptr = &proctab[currpid];
	pptr->waitStart = -1;
	
	int i;
	int semlock_index;
	int hasError = 0;
	int myprio = getprio(currpid);

    for (i = 0; i < numlocks; i++) 
	{
		semlock_index = (int)(*((&ldes1)+i));
		
		//kprintf("semlock_index %d", semlock_index);
		
		// check validity of the lock
		// i) bad semlock_index
		// ii) semlock is free
		// iii) proc does not hold semlock 
		if (isbadsemlock(semlock_index) 
			|| (sptr= &semlock_table[semlock_index])->sstate==LFREE 
			|| pptr->lockbitmask[semlock_index] == 0) 
		{
			hasError = 1;
			continue;
		}
		
		// update some vars associated with this lock
		pptr->lockbitmask[semlock_index] = -pptr->lockbitmask[semlock_index];
		sptr->procbitmask[currpid] = 0;
		
		if(sptr->minholdprio==myprio){
			updateminholdprio(semlock_index);
		}
		
		// current process is releasing the lock, hence decrease count
		if(sptr->type_of_locking==READ){
			sptr->nr--;
		}else if(sptr->type_of_locking==WRITE){
			sptr->nw--;
		}
		
		//kprintf("\n sptr->type_of_locking %d, sptr->nr: %d, sptr->nw: %d, isempty(sptr->sqhead_r):%d", sptr->type_of_locking, sptr->nr, sptr->nw, isempty(sptr->sqhead_r));
		
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
			
			//kprintf("Case 1");

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
							
							/*
							What happens here is:
							1. if reader comes first (has longer waiting time), then give lock to reader
							2. if writer comes first (has longer waiting time):
							2.1 if waiting time is not more than 0.5 seconds longer than reader --> give lock to reader.
							2.2 if waiting time is more than 0.5 seconds longer than reader --> give lock to writer.
							*/
							
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
			
	}// end of the lock iteration loop

	
	// update the priority of the cur process
	// as the current proc is releasing a lock, its new hprio needs to be recomputed
	updatemyprio(currpid);
		
	if(hasError==1){
		restore(ps);
		return(SYSERR);
	}

	restore(ps);

	// as all the eligible process have been put in the ready queue, reschedule 
    resched();

	return(OK);
}


void RemoveReadersFromQueue(int head, int tail, int r_max, int w_max, int writerWaitStart){
	
	while(nonempty(head) && lastkey(tail)>w_max)
		ready(getlast(tail), RESCHNO);
	
	while(nonempty(head) && lastkey(tail)==w_max)
	{
		
		int r_max_proc_id = lastid(tail);
		struct	pentry	*pptr = &proctab[r_max_proc_id];
		
		int r_waittime = (ctr1000 - pptr->waitStart);
		int w_waittime = (ctr1000 - writerWaitStart);
		
		
		if(r_waittime>=w_waittime || (w_waittime - r_waittime)<=500 || writerWaitStart==-1)
		{
			// allow reader
			ready(getlast(tail), RESCHNO);
		}
		else
		{
			break;
		}
	}
	
}

void RemoveWriterFromQueue(int head, int tail){
	
	// remove the highest priority writer from the queue
	if(nonempty(head))
		ready(getlast(tail), RESCHNO);
	
}

void updateminholdprio(int semIndex){
	// update the min hold prio in the lock semIndex
	int j;
	int count = 0;
	for(j=0; j<NPROC; j++)
	{
		if(semlock_table[semIndex].procbitmask[j] == 1)
		{	
			count++;
			int myprio = getprio(j);
			
			if(semlock_table[semIndex].minholdprio == -1 
				|| myprio<semlock_table[semIndex].minholdprio)
			{
				semlock_table[semIndex].minholdprio = myprio;
			}
		}
	}
	
	if(count == 0){
		semlock_table[semIndex].minholdprio = -1;
	}
}


void updatemyprio(int pid){
	// go through all the locks I hold and find the max waiting process on those locks
	// update hprio if that is greater then original proc
	int i;
	int maxprio = 0;
	
	for (i=0 ; i<NLOCKS ; i++) 
	{
		if(proctab[pid].lockbitmask[i] > 0){
			if(semlock_table[i].maxwaitprio>maxprio){
				maxprio = semlock_table[i].maxwaitprio;
			}
		}
	}
	
	if(maxprio!=0)
	{
		if(maxprio>proctab[pid].pprio)
		{
			proctab[pid].pinh = maxprio;
		}
		else
		{
			proctab[pid].pinh = 0;
		}
		
	}
	else
	{
		proctab[pid].pinh = 0;
	}
}