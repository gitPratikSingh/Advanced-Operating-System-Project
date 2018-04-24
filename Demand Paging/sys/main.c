#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>
#include <paging.h>


#define PROC1_VADDR	0x40000000
#define PROC1_VPNO      0x40000
#define PROC2_VADDR     0x80000000
#define PROC2_VPNO      0x80000
#define TEST1_BS	1

#define TPASSED    1
#define TFAILED    0

#define MYVADDR1   0x40000000
#define MYVPNO1    0x40000
#define MYVADDR2   0x80000000
#define MYVPNO2    0x80000
#define MYBS1      1
#define MAX_BSTORE 16

#ifndef NBPG
#define NBPG       4096
#endif

#define assert(x,error) if(!(x)){ \
      kprintf(error);\
      return;\
      }
	  
void proc1_test1(char *msg, int lck) {
	char *addr;
	int i;

	int ret = get_bs(TEST1_BS, 100);
	
	kprintf("getbs %d", ret);
	
	if (xmmap(PROC1_VPNO, TEST1_BS, 100) == SYSERR) {
		kprintf("xmmap call failed\n");
		sleep(3);
		return;
	}

	addr = (char*) PROC1_VADDR;
	for (i = 0; i < 26; i++) {
		*(addr + i * NBPG) = 'A' + i;
	}

	sleep(6);
	
	kprintf("**************\n\n\n\n\n ***********\n");

	for (i = 0; i < 26; i++) {
		kprintf("0x%08x: %c\n", addr + i * NBPG, *(addr + i * NBPG));
		
		unsigned long p, q, adr = PROC1_VADDR + i * NBPG;
		virt_addr_t *virt_addr = (virt_addr_t*)(&adr);
		
		pd_t *pd = (pd_t*)(proctab[currpid].pdbr); // base pagetable entry
		pd = pd + virt_addr->pd_offset; // required pagetable entry
		
		pt_t *pt = (pt_t*)(getaddrfrompage(pd->pd_base));
		pt = pt + virt_addr->pt_offset;

		dump32(adr);
		dump32(virt_addr->pd_offset);		
		dump32(virt_addr->pt_offset);
		
		
		kprintf("\n %c %d found\n\n\n", 'A' + i, pt->pt_acc);
	}

	xmunmap(PROC1_VPNO);
	return;
}

void proc_test2(int i,int j,int* ret,int s) {
  char *addr;
  int bsize;
  int r;
  bsize = get_bs(i, j);
  kprintf("\nafter getbs %d\n", bsm_tab[i].bs_status);
  
  if (bsize != 50){
    	kprintf("2-4 FAILED %d.\n", bsize);
	*ret = TFAILED;
	}
  r = xmmap(MYVPNO1, i, j);
  if (j<=50 && r == SYSERR){
	kprintf("2-5 FAILED.\n");
    	*ret = TFAILED;
  }
  if (j> 50 && r != SYSERR){
	kprintf("2-6 FAILED.\n");
    	*ret = TFAILED;
  }
  if(s>0)
  kprintf("\nBefore Sleep %d\n", bsm_tab[i].bs_status);
  sleep(s);
  if (r != SYSERR)
    xmunmap(MYVPNO1);

  kprintf("\nBefore Release %d\n", bsm_tab[i].ncount);
  release_bs(i);
  kprintf("\Afetr Release %d\n", bsm_tab[i].ncount);
  return;
}
void test2() {
  int pids[16];
  int mypid;
  int i,j;

  int ret = TPASSED;
  kprintf("\nTest 2: Testing backing store operations\n");

  int bsize = get_bs(1, 100);
  if (bsize != 100){
	kprintf("2-1 FAILED.\n");
    	ret = TFAILED;
	}
  release_bs(1);
  bsize = get_bs(1, 130);
  if (bsize != SYSERR){
    	kprintf("2-2 FAILED.\n");
	ret = TFAILED;
	}
  bsize = get_bs(1, 0);
  if (bsize != SYSERR){
    	kprintf("2-3 FAILED.\n");
	ret = TFAILED;
	}

  mypid = create(proc_test2, 2000, 20, "proc_test2", 4, 1,
                 50, &ret, 4);
  resume(mypid);
  sleep(2);
  for(i=1;i<=1;i++){
    pids[i] = create(proc_test2, 2000, 20, "proc_test2", 4, 1,
                     i*20, &ret, 0);
    resume(pids[i]);
  }
  sleep(3);
  kill(mypid);
  for(i=1;i<=5;i++){
    kill(pids[i]);
  }
  if (ret != TPASSED)
    kprintf("\tFAILED!\n");
  else
    kprintf("\tPASSED!\n");
}
void proc1_test3(int i,int* ret) {
  char *addr;
  int bsize;
  int r;

  get_bs(i, 100);
  
  if (xmmap(MYVPNO1, i, 100) == SYSERR) {
    *ret = TFAILED;
    return 0;
  }
  sleep(4);
  xmunmap(MYVPNO1);
  release_bs(i);
  return;
}
void proc2_test3() {
  /*do nothing*/
  sleep(1);
  return;
}
void test3() {
  int pids[16];
  int mypid;
  int i,j;

  int ret = TPASSED;
  kprintf("\nTest 3: Private heap is exclusive\n");

  for(i=0;i<=10;i++){
	  kprintf("%d", i);
    pids[i] = create(proc1_test3, 2000, 20, "proc1_test3", 2, i,&ret);
    if (pids[i] == SYSERR){
      ret = TFAILED;
    }else{
      resume(pids[i]);
    }
  }
  kprintf("\nCVC");
  sleep(1);
  mypid = vcreate(proc2_test3, 2000, 100, 20, "proc2_test3", 0, NULL);
  if (mypid != SYSERR)
    ret = TFAILED;

  for(i=0;i<=15;i++){
    kill(pids[i]);
  }
  if (ret != TPASSED)
    kprintf("\tFAILED!\n");
  else
    kprintf("\tPASSED!\n");
}

void proc1_test2(char *msg, int lck) {
	int *x;

	kprintf("ready to allocate heap space\n");
	x = vgetmem(1024);
	kprintf("heap allocated at %x\n", x);
	*x = 100;
	*(x + 1) = 200;

	kprintf("heap variable: %d %d\n", *x, *(x + 1));
	vfreemem(x, 1024);
}
/*
void proc1_test3(char *msg, int lck) {

	char *addr;
	int i;

	addr = (char*) 0x0;

	for (i = 0; i < 1024; i++) {
		*(addr + i * NBPG) = 'B';
	}

	for (i = 0; i < 1024; i++) {
		kprintf("0x%08x: %c\n", addr + i * NBPG, *(addr + i * NBPG));
	}

	return;
}
*/

void proc1() {

	char *x; 
	char temp; 
	get_bs(1, 100); 
	xmmap(7000, 1, 100);    /* This call simply creates an entry in the backing store mapping */ 
	x = 7000*4096; 
	kprintf("\nstart");
	*x = 'Y';                            /* write into virtual memory, will create a fault and system should proceed as in the prev example */ 
	kprintf("\ndone");
	temp = *x;                    /* read back and check */ 
	kprintf("\nproc 1 %c", temp);
	
	*x = 'Z';
	temp = *x;                    /* read back and check */ 
	kprintf("\nproc 1 %c", temp);
	
	xmunmap(7000);
	kprintf("\Sleep proc1");
	sleep(10);
	kprintf("\nExiting proc1");
}


void proc2() {
	
	kprintf("\Start proc2");
	char *x; 
	char temp_b; 
	xmmap(6000, 1, 100); 
	x = 6000 * 4096; 
	temp_b = *x;
	kprintf("\nproc 2 %c", temp_b);
	xmunmap(6000);
	
}
void test1()
{
  kprintf("\nTest 1: Testing xmmap.\n");
  char *addr1 = (char*)0x40000000; 
  int i = ((unsigned long)addr1) >> 12; 
  
  get_bs(MYBS1, 100);
  if (xmmap(i, MYBS1, 100) == SYSERR) {
    kprintf("xmmap call failed\n");
    kprintf("\tFAILED!\n");
    return;
  }

  for (i = 0; i < 26; i++) {
    *addr1 = 'A'+i;   //trigger page fault for every iteration
    addr1 += NBPG;    //increment by one page each time
  }

  addr1 = (char*)0x40000000;
  for (i = 0; i < 26; i++) {
    if (*addr1 != 'A'+i) {
      kprintf("\tFAILED!\n");
      return;
    }
    addr1 += NBPG;    //increment by one page each time
  }

  xmunmap(0x40000000>>12);
  release_bs(MYBS1);
  kprintf("\tPASSED!\n");
  return;
}

int main() {

	int pid1;
	int pid2;
/*	
	kprintf("\n1: shared memory\n");
	kprintf("\nNull proc dir %d\n", proctab[0].pdbr/NBPG);
	
	pid1 = create(proc1_test1, 2000, 20, "proc1_test1", 0, NULL);
	kprintf("\pid1 proc dir %d\n", proctab[pid1].pdbr/NBPG);
	
	resume(pid1);
	sleep(10);

	kprintf("\n2: vgetmem/vfreemem\n");
	pid1 = vcreate(proc1_test2, 2000, 100, 20, "proc1_test2", 0, NULL);
	kprintf("pid %d has private heap\n", pid1);
	resume(pid1);
	sleep(3);

	kprintf("\n3: Frame test\n");
	pid1 = create(proc1_test3, 2000, 20, "proc1_test3", 0, NULL);
	resume(pid1);
	sleep(3);
*/
	
	/*
	pid1 = create(proc1, 2000, 20, "proc1", 0, NULL);
	pid2 = create(proc2, 2000, 20, "proc2", 0, NULL);
	resume(pid1);
	sleep(5);
	resume(pid2);
	
	*/
	
	test3();
	kprintf("\nExisting main");
	
}