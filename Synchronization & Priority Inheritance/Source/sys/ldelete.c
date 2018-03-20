/* sdelete.c - sdelete */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <lock.h>
#include <stdio.h>

/*------------------------------------------------------------------------
 * sdelete  --  delete a semaphore by releasing its table entry
 *------------------------------------------------------------------------
 */
SYSCALL ldelete(int lockdescriptor)
{
	STATWORD ps;    
	int	pid;
	struct	semlock	*sptr;

	disable(ps);
	if (isbadsemlock(lockdescriptor) || semlock_table[lockdescriptor].sstate==LFREE) {
		restore(ps);
		return(SYSERR);
	}
	sptr = &semlock_table[lockdescriptor];
	sptr->sstate = LFREE;
	sptr->type_of_locking = NOLOCK;
	sptr->minholdprio = -1;
	sptr->maxwaitprio = -1;
	
	// todo
	// update proc priorities of the processes which held this lock
	
	if (nonempty(sptr->sqhead_r) || nonempty(sptr->sqhead_w)) {
		if(nonempty(sptr->sqhead_r)){
			while( (pid=getfirst(sptr->sqhead_r)) != EMPTY)
			  {
				proctab[pid].lwaitret = DELETED;
				proctab[pid].waitStart = -1;
				ready(pid,RESCHNO);
			  }
		}
		
		if(nonempty(sptr->sqhead_w)){
			while( (pid=getfirst(sptr->sqhead_w)) != EMPTY)
			  {
				proctab[pid].lwaitret = DELETED;
				proctab[pid].waitStart = -1;
				ready(pid,RESCHNO);
			  }
		}
		
		
		resched();
	}
	
	restore(ps);
	return(OK);
}
