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
int mystrncmp(char* des,char* target,int n){
    int i;
    for (i=0;i<n;i++){
        if (target[i] == '.') continue;
        if (des[i] != target[i]) return 1;
    }
    return 0;
}

char buffer[3];
int buf_idx; 
void lreader (char *msg, int lck, char c)
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

void lwriter (char *msg, int lck, char c)
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

void sreader (char *msg, int sem, char c)
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

void swriter (char *msg, int sem, char c)
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

void test ()
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


/*--------------------------------Test 1--------------------------------*/
void reader0 (char *msg, int lck)
{
	lock (lck, READ, DEFAULT_LOCK_PRIO);
	kprintf ("  %s: acquired lock, sleep 2s\n", msg);
	sleep (1);
	kprintf ("  %s: to release lock\n", msg);
	releaseall (1, lck);
	
	sleep(10);
	
	
	kprintf ("\n  %s: Trying to acquire deleted lock\n", msg);
	int r = lock (lck, READ, DEFAULT_LOCK_PRIO);
	kprintf("Ret val, r == SYSERR = %d", r == SYSERR);
	
	assert (r == SYSERR, "Test 0 failed");
	
	sleep (1);
	kprintf ("  %s: to release lock\n", msg);
	r = releaseall (1, lck);
	assert (r == SYSERR, "Test 0 failed");
}

void test0 ()
{
	int	lck;
	int	pid1;
	int	pid2;

	kprintf("\nTest 0: delete lock\n");
	lck  = lcreate ();
	assert (lck != SYSERR, "Test 0 failed");

	pid1 = create(reader0, 2000, 20, "reader a", 2, "reader a", lck);
	//pid2 = create(reader1, 2000, 20, "reader b", 2, "reader b", lck);

	resume(pid1);
	//resume(pid2);
	
	sleep (4);
	kprintf("\ndeleting lock %d\n", lck);
	ldelete (lck);
	
	next_available_lock++;
	int nlck  = lcreate ();
	assert (nlck != SYSERR, "Test 0 failed");
	
	kprintf("\ncreated new lock %d\n", nlck);
	
	sleep (10);
	kprintf ("Test 0 ok\n");
}

 
void reader1 (char *msg, int lck)
{
	lock (lck, READ, DEFAULT_LOCK_PRIO);
	kprintf ("  %s: acquired lock, sleep 2s\n", msg);
	sleep (2);
	kprintf ("  %s: to release lock\n", msg);
	releaseall (1, lck);
}

void test1 ()
{
	int	lck;
	int	pid1;
	int	pid2;

	kprintf("\nTest 1: readers can share the rwlock\n");
	lck  = lcreate ();
	assert (lck != SYSERR, "Test 1 failed");

	pid1 = create(reader1, 2000, 20, "reader a", 2, "reader a", lck);
	pid2 = create(reader1, 2000, 20, "reader b", 2, "reader b", lck);

	resume(pid1);
	resume(pid2);
	
	sleep (5);
	ldelete (lck);
	kprintf ("Test 1 ok\n");
}


 
void reader11 (char *msg, int lck, int pr)
{
	kprintf (" pid:%d %s: acquiring lock %d\n", currpid, msg, lck);
	lock (lck, READ, DEFAULT_LOCK_PRIO);
	kprintf (" pid:%d %s: acquired lock, sleep 2s\n", currpid, msg);
	sleep (2);
	kprintf (" pid:%d %s: to release lock\n", currpid, msg);
	releaseall (1, lck);
}

void writer11 (char *msg, int lck)
{
	kprintf (" pid:%d %s: acquiring lock %d\n", currpid, msg, lck);
	lock (lck, WRITE, DEFAULT_LOCK_PRIO);
	kprintf (" pid:%d %s: acquired lock, sleep 2s\n", currpid, msg);
	sleep (7);
	kprintf (" pid:%d %s: to release lock\n", currpid, msg);
	releaseall (1, lck);
}


void test11 ()
{
	int	lck;
	int	pid1;
	int	pid2;
	int	pid3;

	kprintf("\nTest 11: custom\n");
	lck  = lcreate ();
	assert (lck != SYSERR, "Test 1 failed");

	pid1 = create(reader11, 2000, 20, "reader a", 3, "reader a", lck, 25);
	pid2 = create(writer11, 2000, 20, "writer b", 3, "writer b", lck, 25);
	pid3 = create(writer11, 2000, 20, "writer a", 2, "writer a", lck);
	
	resume(pid3);
	sleep(1);
	resume(pid2);
	resume(pid1);
	
	sleep (20);
	ldelete (lck);
	kprintf ("Test 11 ok\n");
}

/*----------------------------------Test 2---------------------------*/
char output2[10];
int count2;
void reader2 (char msg, int lck, int lprio)
{
        int     ret;

        kprintf ("  %c: to acquire lock\n", msg);
        lock (lck, READ, lprio);
        output2[count2++]=msg;
        kprintf ("  %c: acquired lock, sleep 3s\n", msg);
        sleep (3);
        output2[count2++]=msg;
        kprintf ("  %c: to release lock\n", msg);
	releaseall (1, lck);
}

void writer2 (char msg, int lck, int lprio)
{
	kprintf ("  %c: to acquire lock\n", msg);
        lock (lck, WRITE, lprio);
        output2[count2++]=msg;
        kprintf ("  %c: acquired lock, sleep 3s\n", msg);
        sleep (3);
        output2[count2++]=msg;
        kprintf ("  %c: to release lock\n", msg);
        releaseall (1, lck);
}

void test2 ()
{
        count2 = 0;
        int     lck;
        int     rd1, rd2, rd3, rd4;
        int     wr1;

        kprintf("\nTest 2: wait on locks with priority. Expected order of"
		" lock acquisition is: reader A, reader B, reader D, writer C & reader E\n");
        lck  = lcreate ();
        assert (lck != SYSERR, "Test 2 failed");

	rd1 = create(reader2, 2000, 20, "reader2", 3, 'A', lck, 20);
	rd2 = create(reader2, 2000, 20, "reader2", 3, 'B', lck, 30);
	rd3 = create(reader2, 2000, 20, "reader2", 3, 'D', lck, 25);
	rd4 = create(reader2, 2000, 20, "reader2", 3, 'E', lck, 20);
        wr1 = create(writer2, 2000, 20, "writer2", 3, 'C', lck, 25);
	
        kprintf("-start reader A, then sleep 1s. lock granted to reader A\n");
        resume(rd1);
        sleep (1);

        kprintf("-start writer C, then sleep 1s. writer waits for the lock\n");
        resume(wr1);
        sleep10 (1);


        kprintf("-start reader B, D, E. reader B is granted lock.\n");
        resume (rd2);
	resume (rd3);
	resume (rd4);


        sleep (15);
        kprintf("output=%s\n", output2);
        assert(mystrncmp(output2,"ABABDDCCEE",10)==0,"Test 2 FAILED\n");
        kprintf ("Test 2 OK\n");
}

/*----------------------------------Test 3---------------------------*/
void reader3 (char *msg, int lck)
{
        int     ret;

        kprintf ("  %s: to acquire lock\n", msg);
        lock (lck, READ, DEFAULT_LOCK_PRIO);
        kprintf ("  %s: acquired lock\n", msg);
        kprintf ("  %s: to release lock\n", msg);
        releaseall (1, lck);
}

void writer3 (char *msg, int lck)
{
        kprintf ("  %s: to acquire lock\n", msg);
        lock (lck, WRITE, DEFAULT_LOCK_PRIO);
        kprintf ("  %s: acquired lock, sleep 10s\n", msg);
        sleep (10);
        kprintf ("  %s: to release lock\n", msg);
        releaseall (1, lck);
}

void test3 ()
{
        int     lck;
        int     rd1, rd2;
        int     wr1;

        kprintf("\nTest 3: test the basic priority inheritence\n");
        lck  = lcreate ();
        assert (lck != SYSERR, "Test 3 failed");

        rd1 = create(reader3, 2000, 25, "reader3", 2, "reader A", lck);
        rd2 = create(reader3, 2000, 30, "reader3", 2, "reader B", lck);
        wr1 = create(writer3, 2000, 20, "writer3", 2, "writer", lck);
		
		kprintf("rd1 %d, rd2 %d, wr1 %d, main %d", rd1, rd2, wr1, currpid);
		kprintf("\nsemlock_table[lockid]:%d\n", semlock_table[lck].procbitmask[32]);
		
        kprintf("-start writer, then sleep 1s. lock granted to write (prio 20)\n");
        resume(wr1);
        sleep (1);

        kprintf("-start reader A, then sleep 1s. reader A(prio 25) blocked on the lock\n");
        resume(rd1);
        sleep (1);
		
	assert (getprio(wr1) == 25, "Test 3 failed");

        kprintf("-start reader B, then sleep 1s. reader B(prio 30) blocked on the lock\n");
        resume (rd2);
	sleep (1);
	assert (getprio(wr1) == 30, "Test 3 failed");
	
	kprintf("-kill reader B, then sleep 1s\n");
	kill (rd2);
	sleep (1);
	assert (getprio(wr1) == 25, "Test 3 failed");
	
	kprintf("-Change prio reader A, then sleep 1s\n");
	chprio(rd1, 35);
	sleep(1);
	assert(getprio(wr1) == 35, "Test 3 failed");
	
	kprintf("-kill reader A, then sleep 1s\n");
	kill (rd1);
	sleep(1);
	
	kprintf("Wr1 prio4 : %d", getprio(wr1));
	assert(getprio(wr1) == 20, "Test 3 failed");

        sleep (8);
        kprintf ("Test 3 OK\n");
}

/*----------------------------------Test 4---------------------------*/
void reader4 (char *msg, int lckhold)
{
        int     ret;

        kprintf ("  %s: to acquire lock %d\n", msg, lckhold);
        lock (lckhold, READ, DEFAULT_LOCK_PRIO);
        kprintf ("  %s: acquired lock %d, sleep 10s\n", msg, lckhold);
		sleep (10);
		kprintf ("  %s: to release lock %d\n", msg, lckhold);
        releaseall (1, lckhold);
}

void writer4 (char *msg, int lck, int lock1)
{
        kprintf ("  %s: to acquire lock %d\n", msg, lck);
        lock (lck, WRITE, DEFAULT_LOCK_PRIO);
        kprintf ("  %s: acquired lock %d \n", msg, lck);
		
        sleep (2);
		
		kprintf ("  %s: to acquire lock %d\n", msg, lock1);
        lock (lock1, WRITE, DEFAULT_LOCK_PRIO);
        kprintf ("  %s: acquired lock %d \n", msg, lock1);
		
        kprintf ("  %s: to release locks %d and %d\n", msg, lck, lock1);
        releaseall (2, lck, lock1);
}

void test4 ()
{
        int     lck1;
		int     lck2;
        int     rd1, rd2;
        int     wr1;

        kprintf("\nTest 3: test the basic priority inheritence\n");
        lck1  = lcreate ();
        lck2  = lcreate ();
        
		assert (lck1 != SYSERR, "Test 3 failed");
		assert (lck2 != SYSERR, "Test 3 failed");
		
        rd1 = create(reader4, 2000, 10, "reader4", 2, "reader A", lck2);
        rd2 = create(reader4, 2000, 30, "reader4", 2, "reader C", lck1);
        wr1 = create(writer4, 2000, 20, "writer4", 3, "writer B", lck1, lck2);
		
		kprintf("rd1 %d, rd2 %d, wr1 %d, main %d", rd1, rd2, wr1, currpid);
		
        kprintf("-start reader 1, then sleep 1s. lock granted to Read (prio 10)\n");
        resume(rd1);
        sleep (1);

        kprintf("-start Writer 1, then sleep 1s. reader A(prio 20) blocked on the lock\n");
        resume(wr1);
        sleep (5);
		
		kprintf("\n getprio(rd1) %d", getprio(rd1));
		kprintf("\n lock %d stats maxwaitprio: %d", lck2, semlock_table[lck2].maxwaitprio);
		
		
		assert (getprio(rd1) == 20, "Test 4 failed");

        kprintf("-start reader 2, then sleep 1s. reader B(prio 30) blocked on the lock\n");
        resume (rd2);
		sleep (1);
	
		assert (getprio(rd1) == 30, "Test 3 failed");
		assert (getprio(wr1) == 30, "Test 3 failed");
		/*
	kprintf("-kill reader B, then sleep 1s\n");
	kill (rd2);
	sleep (1);
	assert (getprio(wr1) == 25, "Test 3 failed");
	
	kprintf("-Change prio reader A, then sleep 1s\n");
	chprio(rd1, 35);
	sleep(1);
	assert(getprio(wr1) == 35, "Test 3 failed");
	
	kprintf("-kill reader A, then sleep 1s\n");
	kill (rd1);
	sleep(1);
	
	kprintf("Wr1 prio4 : %d", getprio(wr1));
	assert(getprio(wr1) == 20, "Test 3 failed");
	*/
        sleep (8);
        kprintf ("Test 4 OK\n");
}

int main( )
{
        /* These test cases are only used for test purpose.
         * The provided results do not guarantee your correctness.
         * You need to read the PA2 instruction carefully.
         */
		kprintf("sptr->sstate == LFREE?%d", (&semlock_table[0])->sstate == LFREE);

	//test1();

	//test2();
	
	//test3();
	
	//test4();
	
	//test0();
	
	test11();
	
	//task1();

        /* The hook to shutdown QEMU for process-like execution of XINU.
         * This API call exists the QEMU process.
         */
        shutdown();
}



