/* resched.c  -  resched */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <lab1.h>

unsigned long currSP;	/* REAL sp of current process */
extern int ctxsw(int, int, int, int);
/*-----------------------------------------------------------------------
 *  * resched  --  reschedule processor to highest priority ready process
 *   *
 *    * Notes:	Upon entry, currpid gives current process id.
 *     *		Proctab[currpid].pstate gives correct NEXT state for
 *      *			current process if other than PRREADY.
 *       *------------------------------------------------------------------------
 *        */
int epoch = 0;
int globalschedclass = -1;

void setschedclass(int sched_class){
	globalschedclass = sched_class;
}

int getschedclass(){
	return globalschedclass;
}

LOCAL int getProcFromRandomScheduler();
int resched()
{
	register struct	pentry	*optr;	/* pointer to old process entry */
	register struct	pentry	*nptr;	/* pointer to new process entry */


	/* no switch needed if current process priority higher than next */
	int schedclass = getschedclass();

	if(schedclass == RANDOMSCHED){
		optr= &proctab[currpid];

		if (optr->pstate == PRCURR) {
					optr->pstate = PRREADY;
					insert(currpid,rdyhead,optr->pprio);
			}
		
		nptr = &proctab[currpid = getProcFromRandomScheduler()];	
		nptr->pstate = PRCURR;
		
	#ifdef	RTCLOCK
		preempt = QUANTUM;		/* reset preemption counter	*/
	#endif


	}else if(schedclass == LINUXSCHED){
		// recompute 
		
		struct pentry *pArray = &proctab[0];
		int i, tempMaxGoodness = 0;
		int indexOfHighestGoodnessRunnableProc = 0;
		
		int beginNewEpoch = 0;
		int newEpochVal = 0;
		
		epoch = epoch - (proctab[currpid].counter - preempt);
		
		if(epoch<=0 || currpid == NULLPROC){
			beginNewEpoch = 1;
		}
		
		proctab[currpid].counter = preempt; 
		
		for(i=NPROC-1; i>0; i--){
			//loop over all the processes
			if((pArray+i)->pstate != PRFREE){
				// if its a new process, it will not get scheduled until the next epoch 
				if((pArray+i)->isnew == 0 || beginNewEpoch == 1){

					if(beginNewEpoch == 1){
						// NEW EPOCH
						if((pArray+i)->quantum == (pArray+i)->counter){
							//proc has not used up its quantum at all
							(pArray+i)->quantum = (pArray+i)->pprio;
						}
						else{
							(pArray+i)->quantum = ((pArray+i)->counter)/2 + (pArray+i)->pprio;
						}
						
						(pArray+i)->counter = (pArray+i)->quantum;
						(pArray+i)->goodness = (pArray+i)->counter + (pArray+i)->pprio;
						
						(pArray+i)->isnew = 0;
						// proc priority changes will come into affect only from next epoch
						(pArray+i)->oldprio = (pArray+i)->pprio; 
						newEpochVal = newEpochVal + (pArray+i)->quantum;
					
					}else{
						//OLD EPOCH, just compute goodness of all the procs
						if((pArray+i)->counter==0)
							(pArray+i)->goodness = 0;
						else
							(pArray+i)->goodness = (pArray+i)->counter + (pArray+i)->oldprio;
					}
					
					if( (pArray+i)->goodness > 0 
						&& tempMaxGoodness<((pArray+i)->goodness)
						&& ((pArray+i)->pstate==PRCURR || (pArray+i)->pstate==PRREADY)){
						tempMaxGoodness = (pArray+i)->goodness;
						indexOfHighestGoodnessRunnableProc = i;
					}
				}
			}
		}
		
		if(beginNewEpoch == 1){
			epoch = newEpochVal;
		}
		
		optr= &proctab[currpid];

		if (optr->pstate == PRCURR) {
			optr->pstate = PRREADY;
			insert(currpid,rdyhead,optr->pprio);
		}
			
		// remove the process from the ready queue
		dequeue(indexOfHighestGoodnessRunnableProc);
		nptr = &proctab[currpid = indexOfHighestGoodnessRunnableProc];
		nptr->pstate = PRCURR;
		
		#ifdef	RTCLOCK
			if(indexOfHighestGoodnessRunnableProc == NULLPROC){
				preempt = QUANTUM;
			}else{
				preempt = nptr->counter;
			}		
		#endif
			
	}else{

			if ( ( (optr= &proctab[currpid])->pstate == PRCURR) 
				&& (lastkey(rdytail)<optr->pprio)) {
				return(OK);
			}
			
			/* force context switch */
			if (optr->pstate == PRCURR) {
				optr->pstate = PRREADY;
				insert(currpid,rdyhead,optr->pprio);
			}

			/* remove highest priority process at end of ready list */
			nptr = &proctab[ (currpid = getlast(rdytail)) ];
			nptr->pstate = PRCURR;		/* mark it currently running	*/
			
		#ifdef	RTCLOCK
			preempt = QUANTUM;		/* reset preemption counter	*/
		#endif
			
	}
	
	ctxsw((int)&optr->pesp, (int)optr->pirmask, (int)&nptr->pesp, (int)nptr->pirmask);
	
	/* The OLD process returns here when resumed. */
	return OK;
}

LOCAL int getProcFromRandomScheduler(){
	struct pentry *pArray = &proctab[0];
	
	int i;
	int sumProcPriority = 0;
    for(i=NPROC-1; i>=0; i--){
		if((pArray+i)->pstate == PRCURR || (pArray+i)->pstate == PRREADY){
			sumProcPriority = sumProcPriority + (pArray+i)->pprio;
		}
	}
	
	/* Use current time as seed for random generator*/
    	// srand(time(0));
	
	/* randNum will always be between 0, sumProcPriority-1*/
	int randNum = rand();
	
	// avoid % by 0
	if(sumProcPriority!=0)
		randNum = randNum%sumProcPriority;
	else
		randNum = 0;
	
	int proc = q[rdytail].qprev;
	
	while (q[proc].qkey < randNum)
	{	
		randNum = randNum - q[proc].qkey;
		proc = q[proc].qprev;
	}
	
	
	// remove the process from the ready queue
	dequeue(proc);
	
	return proc;
}
