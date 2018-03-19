#include <conf.h>
#include <kernel.h>
#include <q.h>
#include <sched.h>
#include <stdio.h>
#include <proc.h>
#define LOOP	50

int prA, prB, prC;
int proc_a(char), proc_b(char), proc_c(char);
int proc(char);
volatile int a_cnt = 0;
volatile int b_cnt = 0;
volatile int c_cnt = 0;
volatile int s = 0;

int globalSchedClass = -1;

void setschedclass(int sched_class){
	globalSchedClass = 1;
}

int getschedclass(){
	return globalSchedClass;
}

int main_proc =0;
struct	pentry	*pmain;

int main()
{
	int i;

	kprintf("\n\nTEST1:\n");
	resume(prA = create(proc_a,2000,20,"proc A",1,'A'));
	resume(prB = create(proc_b,2000,20,"proc B",1,'B'));
	resume(prC = create(proc_c,2000,20,"proc C",1,'C'));

	while (1) {
		kprintf("%c", 'D');
		for (i=0; i< 10000; i++);
	}
}

int main1() {
	main_proc = currpid;
	pmain = &proctab[main_proc];
	
	kprintf("mainproc stat: %d %d %d %d %d %d %d %d",
pmain->pstate==PRCURR,
pmain->pstate==PRFREE,
pmain->pstate==PRREADY,
pmain->pstate==PRRECV,
pmain->pstate==PRSLEEP,
pmain->pstate==PRSUSP,
pmain->pstate==PRWAIT,
pmain->pstate==PRTRECV
);
	int i;
	int count = 0;
	char buf[8];

	srand(1234);

	kprintf("Please Input:\n");
	while ((i = read(CONSOLE, buf, sizeof(buf))) < 1)
		;
	buf[i] = 0;
	s = atoi(buf);
	kprintf("Get %d\n", s);
	
	if (0) {
		setschedclass(RANDOMSCHED);
		prA = create(proc_a, 2000, 10, "proc A", 1, 'A');
	//	prB = create(proc_b, 2000, 20, "proc B", 1, 'B');
	//	prC = create(proc_c, 2000, 30, "proc C", 1, 'C');
		
		kprintf("Created all the process\n\n");
		resume(prA);
	//	resume(prB);
	//	resume(prC);
		kprintf("Resumed all processes\n\n");

		sleep(10);
		kprintf("got backfrom sleep\n");

		kill(prA);
		//kill(prB);
		//kill(prC);

		kprintf("\nTest Result: A = %d, B = %d, C = %d\n", a_cnt, b_cnt, c_cnt);
	}
	else if(0) {
		//setschedclass(LINUXSCHED);
		resume(prA = create(proc, 2000, 5, "proc A", 1, 'A'));
		resume(prB = create(proc, 2000, 50, "proc B", 1, 'B'));
		resume(prC = create(proc, 2000, 90, "proc C", 1, 'C'));

		while (count++ < LOOP) {
			kprintf("M");
			for (i = 0; i < 1000000; i++)
				;
		}
        kprintf("\n");
	}
	return 0;
}

int proc_a(char c) {
	int i;
	kprintf("Start... %c\n", c);
	b_cnt = 0;
	c_cnt = 0;

	kprintf("mainproc stat: %d %d %d %d %d %d %d %d",
		pmain->pstate==PRCURR,
		pmain->pstate==PRFREE,
		pmain->pstate==PRREADY,
		pmain->pstate==PRRECV,
		pmain->pstate==PRSLEEP,
		pmain->pstate==PRSUSP,
		pmain->pstate==PRWAIT,
		pmain->pstate==PRTRECV
	);
	
	while (1) {
		for (i = 0; i < 10000; i++)
			;
		a_cnt++;
         //       kprintf("Main Proc Status: %c ", proctab[main_proc].pstate);
        /* 
	 kprintf("*** mainproc stat: %d %d %d %d %d %d %d %d ***** ",
                pmain->pstate==PRCURR,
                pmain->pstate==PRFREE,
                pmain->pstate==PRREADY,
                pmain->pstate==PRRECV,
                pmain->pstate==PRSLEEP,
                pmain->pstate==PRSUSP,
                pmain->pstate==PRWAIT,
                pmain->pstate==PRTRECV
        );
*/

	}
	return 0;
}

int proc_b(char c) {
	int i;
	kprintf("Start... %c %d \n", c, main_proc);
	a_cnt = 0;
	c_cnt = 0;

	while (1) {
		for (i = 0; i < 10000; i++)
			;
		b_cnt++;
		
		kprintf("Main Proc Status: %c ", proctab[main_proc].pstate);
	}
	return 0;
}

int proc_c(char c) {
	int i;
	kprintf("Start... %c\n", c);
	a_cnt = 0;
	b_cnt = 0;

	while (1) {
		for (i = 0; i < 10000; i++)
			;
		c_cnt++;
	}
	return 0;
}

int proc(char c) {
	int i;
	int count = 0;

	while (count++ < LOOP) {
		kprintf("%c", c);
		for (i = 0; i < 1000000; i++)
			;
	}
	return 0;
}

