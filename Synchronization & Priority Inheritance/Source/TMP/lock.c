#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <stdio.h>
#include <lock.h>

void updatepriority(int[], int);

int lock (int ldes1, int type, int priority){
	STATWORD ps;    
	struct	semlock	*sptr;
	struct	pentry	*pptr;

	disable(ps);
	if (isbadsemlock(ldes1) // invalid lock
		|| (sptr= &semlock_table[ldes1])->sstate==LFREE // lock is not created 
		|| (proctab[currpid].lockbitmask[ldes1] > 0) // process already holds the locks
		) {
		restore(ps);
		return(SYSERR);
	}
	
	// match lock version# 
	// if this is a second request for the same lock, then lockbitmask will have the last lock version no
	if(proctab[currpid].lockbitmask[ldes1]<0){
		
		if(-1 * proctab[currpid].lockbitmask[ldes1] != sptr->lockversion){
			restore(ps);
			return(SYSERR);
		}
	}
	
	// some useful vars
	int IsSafeState = ((sptr->nr == 0 || sptr->nw == 0) && (sptr->nw <= 1));
	
	/*
	if(IsSafeState==0){
		kprintf("\nIsSafeState Error!!!");
	}
	*/
	
	int myprio = getprio(currpid);
	
	if(type == READ){
		// no readers should be kept waiting unless
		// (i) a writer has already obtained the lock, or 
		// (ii) there is a higher lock priority writer already waiting for the lock
		// otherwise grant the lock
		if(sptr->type_of_locking == WRITE){
			// case(i)
			insert(currpid, sptr->sqhead_r, priority);
		}
		else
		{
			// nolock/read lock
			int maxPriorityInWriterQueue = lastkey(sptr->sqtail_w);
			if(maxPriorityInWriterQueue>priority)
			{
				// case (ii)
				insert(currpid, sptr->sqhead_r, priority);
			}
			else
			{
				// grant read access
				sptr->nr++;
				sptr->type_of_locking = READ;
				
				// update some vars associated with this lock
				sptr->procbitmask[currpid] = 1;
				proctab[currpid].lockbitmask[ldes1] = sptr->lockversion;
				
				if(sptr->minholdprio==-1 || sptr->minholdprio>myprio){
					sptr->minholdprio = myprio;
				}
				
				restore(ps);
				return (OK);
			}
		}	
	}
	else
	// request for write lock
	{
		// grant only if there is no active reader/writer 
		// and if the priority is higher than all the waiting processes
		if(sptr->type_of_locking != NOLOCK){
			// case(i)
			insert(currpid, sptr->sqhead_w, priority);
			
		}else{
			int maxPriorityInWriterQueue = lastkey(sptr->sqtail_w);
			int maxPriorityInReaderQueue = lastkey(sptr->sqtail_r);
			
			if(priority>maxPriorityInWriterQueue && priority>maxPriorityInReaderQueue){
				// grant access to writer
				sptr->nw++;
				sptr->type_of_locking = WRITE;
				
				// update some vars associated with this lock
				sptr->procbitmask[currpid] = 1;
				proctab[currpid].lockbitmask[ldes1] = sptr->lockversion;
				
				if(sptr->minholdprio==-1 || sptr->minholdprio>myprio){
					sptr->minholdprio = myprio;
				}
				
				restore(ps);
				return (OK);
			}else{
				
				insert(currpid, sptr->sqhead_w, priority);
			}
		}
	}
	
	
	// if the control moves to this part, it means the cur process needs to wait for the lock! 
	(pptr = &proctab[currpid])->pstate = PRLOCKED;
	pptr->lwaitret = OK;
	pptr->waitStart = ctr1000;
	pptr->lockid = ldes1;
	

	// before resheduling update the priority of the procs holding the lock if any of their prio is less than this proc's prio
	if(myprio > sptr->minholdprio){
		updatepriority(sptr->procbitmask, myprio);
		sptr->minholdprio = myprio;
	}
	
	//update the max wait prio as well
	if(myprio > sptr->maxwaitprio){
		sptr->maxwaitprio = myprio;
	}
	
	resched();
	
	
	// lock granted	
	// update some useful variables
	sptr->type_of_locking=type;
	sptr->procbitmask[currpid] = 1;
	pptr->lockid = -1;
	pptr->lockbitmask[ldes1] = sptr->lockversion;
	
	if(sptr->type_of_locking==READ){
		sptr->nr++;
	}else if(sptr->type_of_locking==WRITE){
		sptr->nw++;
	}
	
	//update the min hold prio
	if(myprio == sptr->minholdprio){
		updateminholdprio(ldes1);
	}
	
	//update the max wait prio as well
	if(myprio == sptr->maxwaitprio){
		updatemaxwaitprio(ldes1);
	}
	
	// Requirement: Waiting on a lock will return a new constant DELETED instead of OK when returning due to a deleted lock
	// pptr->lwaitret will store DELETED if the lock was deleted
	restore(ps);
	return pptr->lwaitret;	
}

void updatemaxwaitprio(int semIndex){
	// update the min hold prio in the lock semIndex
	int j;
	int max = -1;
	for(j=0; j<NPROC; j++)
	{
		if(proctab[j].lockid == semIndex){
			int myprio = getprio(j);
			if(max < myprio){
				max = myprio;
			}
		}
	}
	
	semlock_table[semIndex].maxwaitprio = max;
}

void updatepriority(int procbitmask[], int minprio)
{	
	int j;
	for(j=0; j<NPROC; j++)
	{	// update the prio of the procs holding the lock
		if(procbitmask[j] == 1 && getprio(j)<minprio)
		{
			//kprintf("\nupdatepriority J: %d, procname: %s\n", j, proctab[j].pname);
			proctab[j].pinh = minprio;
			// ensure transitivity of prio update 
			// i.e if this process is waiting for a lock some other procs update their prio
			if(proctab[j].pstate == PRLOCKED && proctab[j].lockid != -1){
				int lockid = proctab[j].lockid;
				if(semlock_table[lockid].minholdprio<minprio){
					updatepriority(semlock_table[lockid].procbitmask, minprio);
					semlock_table[lockid].minholdprio = minprio;
				}
			}
		}
	}
}