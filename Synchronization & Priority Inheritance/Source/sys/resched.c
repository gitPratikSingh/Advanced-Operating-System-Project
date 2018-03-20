/* resched.c  -  resched */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>

unsigned long currSP;	/* REAL sp of current process */
extern int ctxsw(int, int, int, int);
/*-----------------------------------------------------------------------
 * resched  --  reschedule processor to highest priority ready process
 *
 * Notes:	Upon entry, currpid gives current process id.
 *		Proctab[currpid].pstate gives correct NEXT state for
 *			current process if other than PRREADY.
 *------------------------------------------------------------------------
 */
int resched()
{
	register struct	pentry	*optr;	/* pointer to old process entry */
	register struct	pentry	*nptr;	/* pointer to new process entry */

	/* no switch needed if current process priority higher than next*/
	optr= &proctab[currpid];
	
	if ( ( optr->pstate == PRCURR) && isempty(rdyhead)) {
		return(OK);
	}
	
	/* force context switch */

	if (optr->pstate == PRCURR) {
		optr->pstate = PRREADY;
		insert(currpid,rdyhead,optr->pprio);
	}

	int tmpmaxprio = 0;
	int procwithhighestprio = 0;
	int next = q[rdyhead].qnext;
	
	while (q[next].qkey < MAXINT){	/* tail has maxint as key	*/
		if(tmpmaxprio < getprio(next)){
			tmpmaxprio = getprio(next);
			procwithhighestprio = next;
		}
		next = q[next].qnext;
	}
	
	
	/* remove highest priority process */
	nptr = &proctab[ (currpid = dequeue(procwithhighestprio)) ];

	
	nptr->pstate = PRCURR;		/* mark it currently running	*/
#ifdef	RTCLOCK
	preempt = QUANTUM;		/* reset preemption counter	*/
#endif
	
	ctxsw((int)&optr->pesp, (int)optr->pirmask, (int)&nptr->pesp, (int)nptr->pirmask);
	
	/* The OLD process returns here when resumed. */
	return OK;
}
