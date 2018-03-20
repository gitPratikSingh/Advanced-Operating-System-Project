#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <lock.h>
#include <stdio.h>

#define DEFAULT_LOCK_PRIO 20

#define assert(x,error) if(!(x)){ \
            kprintf(error);\
            return;\
            }

LOCAL int mystrncmp(char* des,char* target,int n){
    int i;
    for (i=0;i<n;i++){
        if (target[i] == '.') continue;
        if (des[i] != target[i]) return 1;
    }
    return 0;
}

char buffer[3];
int buf_idx; 
LOCAL void lreader (char *msg, int lck, char c)
{	
	kprintf ("  %s: trying to acquire lock %d\n", msg, lck);
	lock (lck, READ, DEFAULT_LOCK_PRIO);
	kprintf ("  %s: acquired lock %d, process prio %d, sleep 3s\n", msg,lck, getprio(currpid));
	sleep(5);
	
	int loop;
	for(loop =0; loop<MAXINT; loop++){}
	
	buffer[buf_idx++] = c;
	kprintf ("  %s: to release lock, process prio %d \n", msg , getprio(currpid));
	releaseall (1, lck);
}

LOCAL void lwriter (char *msg, int lck, char c)
{	
	kprintf ("  %s: trying to acquire lock %d\n", msg, lck);
	lock (lck, WRITE, DEFAULT_LOCK_PRIO);
	kprintf ("  %s: acquired lock %d, process prio %d, sleep 3s\n", msg,lck, getprio(currpid));
	
	if(c=='l')
		sleep(3);
	
	int loop;
	for(loop =0; loop<MAXINT; loop++){
	}
	
	buffer[buf_idx++] = c;
	kprintf ("  %s: to release lock, process prio %d \n", msg, getprio(currpid));
	releaseall (1, lck);
}

LOCAL void sreader (char *msg, int sem, char c)
{	
	kprintf ("  %s: trying to acquire sem %d\n", msg, sem);
	wait (sem);
	kprintf ("  %s: acquired sem %d, process prio %d, sleep 3s\n", msg,sem, getprio(currpid));
	sleep(5);
	
	int loop;
	for(loop =0; loop<MAXINT; loop++){}
	
	buffer[buf_idx++] = c;
	kprintf ("  %s: to release sem, process prio %d \n", msg, getprio(currpid));
	signal (sem);
}

LOCAL void swriter (char *msg, int sem, char c)
{	
	kprintf ("  %s: trying to acquire sem %d\n", msg, sem);
	wait (sem);
	kprintf ("  %s: acquired sem %d, process prio %d, sleep 2s\n", msg,sem, getprio(currpid));
	
	if(c=='l')
		sleep(3);
	
	int loop;
	for(loop =0; loop<MAXINT; loop++){
	}
	
	
	buffer[buf_idx++] = c;
	kprintf ("  %s: to release sem, process prio %d \n", msg, getprio(currpid));
	signal (sem);
}

void task1 ()
{
	int	lck1;
	int	lck2;
	int	wr_low;
	int	wr_high;
	int	rd_med;
	int i=0;
	for(i=0; i<3; i++){
		buffer[i] = '0';
	}
	buf_idx = 0;
	
	kprintf("\nTest: Priority Inversion test with locks\n");
	lck1  = lcreate ();
	assert (lck1 != SYSERR, "Test failed");

	lck2  = lcreate ();
	assert (lck2 != SYSERR, "Test failed");

	wr_high = create(lwriter, 2000, 50, "wr_high", 3, "wr_high", lck1, 'h');
	wr_low = create(lwriter, 2000, 30, "wr_low", 3, "wr_low", lck1, 'l');
	rd_med = create(lreader, 2000, 40, "rd_med", 3, "rd_med", lck2, 'm');
	
	kprintf("\nStarting low priority read on lock A\n");
	resume(wr_low);
	kprintf("\nStarting medium priority write on lock B\n");
	resume(rd_med);
	sleep(1);
	kprintf("\nStarting high priority write on lock A\n");
	resume(wr_high);
	
	sleep(2);
	assert(mystrncmp(buffer,"lhm",3)==0,"Priority Inversion test with locks FAILED\n");
	
	kprintf ("\nlow->high->medium\n");
	kprintf ("\nPriority Inversion test with locks ok\n");
	
	
	int	sem1;
	int	sem2;
	for(i=0; i<3; i++){
		buffer[i] = '0';
	}
	buf_idx = 0;
	
	kprintf("\nTest: Priority Inversion test with semaphore\n");
	sem1  = screate (1);
	assert (sem1 != SYSERR, "Test failed");

	sem2  = screate (1);
	assert (sem2 != SYSERR, "Test failed");

	wr_high = create(swriter, 2000, 50, "wr_high", 3, "wr_high", sem1, 'h');
	wr_low = create(swriter, 2000, 30, "wr_low", 3, "wr_low", sem1, 'l');
	rd_med = create(sreader, 2000, 40, "rd_med", 3, "rd_med", sem2, 'm');
	
	kprintf("\nStarting low priority read on sem A\n");
	resume(wr_low);
	kprintf("\nStarting medium priority write on sem B\n");
	resume(rd_med);
	sleep(1);
	kprintf("\nStarting high priority write on sem A\n");
	resume(wr_high);
	
	sleep(2);
	assert(mystrncmp(buffer,"mlh",3)==0,"Priority Inversion test with semaphore FAILED\n");
	
	kprintf ("\nmedium->low->high\n");
	kprintf ("\nPriority Inversion test with semaphore ok\n");
	
}




