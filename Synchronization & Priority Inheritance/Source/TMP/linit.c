#include <kernel.h>
#include <proc.h>
#include <stdio.h>
#include <lock.h>
struct semlock semlock_table[NLOCKS];
int	next_available_lock;

int linit(){
	next_available_lock = NLOCKS-1;
	int i,j;
	struct	semlock	*sptr;
	
	for (i=0 ; i<NLOCKS ; i++) {	/* initialize semaphores */
		
		(sptr = &semlock_table[i])->sstate = LFREE;
		
		sptr->type_of_locking = NOLOCK;
		
		sptr->nr = 0;
		sptr->nw = 0;
		
		sptr->sqtail_r = 1 + (sptr->sqhead_r = newqueue());
		sptr->sqtail_w = 1 + (sptr->sqhead_w = newqueue());
		
		sptr->minholdprio = -1;
		sptr->maxwaitprio = -1;
		
		for(j=0; j<NPROC; j++)
		{
			sptr->procbitmask[j] = 0;
		}
		
		sptr->lockversion = 0;
	}
	
	return 0;
}
