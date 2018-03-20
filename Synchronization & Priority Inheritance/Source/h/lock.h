#define NLOCKS 50

#define	LFREE	'\01'		/* this semaphore is free		*/
#define	LUSED	'\02'		/* this semaphore is used		*/

#define READ 2
#define WRITE 3
#define NOLOCK 0

struct	semlock	{		/* semaphore table entry		*/
	char sstate;		/* the state LFREE or LUSED		*/
	
	int type_of_locking; /* nolock read lock or write lock*/ 
	
	int	nr;		/*  # of active readers for this semaphore	*/
	int nw;		/*  # of active writers for this semaphore	*/
	
	int	sqhead_r;		/* q index of head of readers list		*/
	int	sqtail_r;		/* q index of tail of readers list		*/
	
	int	sqhead_w;		/* q index of head of writers list		*/
	int	sqtail_w;		/* q index of tail of writers list		*/
	
	int	procbitmask[NPROC]; /* bitmask of the process ids of the processes currently holding the lock */
	int minholdprio;	/* min priority of the process holding the lock*/
	int maxwaitprio;	/* max priority of the process waiting on the lock*/
	
	int lockversion;		/* version of the lock start from 1,2...*/
};

extern struct semlock semlock_table[];	/* semaphore table			*/
extern	int	next_available_lock;
extern unsigned long ctr1000;

#define	isbadsemlock(s)	(s<0 || s>=NLOCKS)

extern int linit(void);
extern int  lcreate(void);
extern int  ldelete(int lockdescriptor);
extern int  lock(int ldes1, int type, int priority);
extern int  releaseall(int numlocks, int ldes1, ...);
extern void updateminholdprio(int);
extern void updatemaxwaitprio(int);
extern void updatemyprio(int);
extern void RemoveReadersFromQueue(int, int, int, int, int);
extern void RemoveWriterFromQueue(int, int);
extern void resetholdprocpriority(int, int []);
extern void task1();