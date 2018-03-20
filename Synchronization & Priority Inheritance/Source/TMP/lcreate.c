/* screate.c - screate, newsem */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <lock.h>
#include <stdio.h>

LOCAL int newsem();

/*------------------------------------------------------------------------
 * screate  --  create and initialize a semaphore, returning its id
 *------------------------------------------------------------------------
 */
SYSCALL lcreate()
{
	STATWORD ps;    
	int	locker_id, j;
	struct	semlock	*sptr;
	
	disable(ps);
	
	if ( (locker_id=newsem())==SYSERR ) {
		restore(ps);
		return(SYSERR);
	}
	
	// init 
	sptr = &semlock_table[locker_id];
	sptr->type_of_locking = NOLOCK;
	sptr->nr = 0;
	sptr->nw = 0;
	sptr->minholdprio = -1;
	sptr->maxwaitprio = -1;
	for(j=0; j<NPROC; j++)
	{
		sptr->procbitmask[j] = 0;
	}
	
	sptr->lockversion++;
	
	restore(ps);
	
	return(locker_id);
}

/*------------------------------------------------------------------------
 * newsem  --  allocate an unused semaphore and return its index
 *------------------------------------------------------------------------
 */
LOCAL int newsem()
{
	int	sem;
	int	i;

	for (i=0 ; i<NLOCKS ; i++) {
		sem=next_available_lock--;
		if (next_available_lock < 0)
			next_available_lock = NLOCKS-1;
		if (semlock_table[sem].sstate==LFREE) {
			semlock_table[sem].sstate = LUSED;
			return(sem);
		}
	}
	return(SYSERR);
}
