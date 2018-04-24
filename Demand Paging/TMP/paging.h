/* paging.h */

typedef unsigned int	 bsd_t;

/* Structure for a page directory entry */

typedef struct {

  unsigned int pd_pres	: 1;		/* page table present?		*/
  unsigned int pd_write : 1;		/* page is writable?		*/
  unsigned int pd_user	: 1;		/* is use level protection?	*/
  unsigned int pd_pwt	: 1;		/* write through cachine for pt?*/
  unsigned int pd_pcd	: 1;		/* cache disable for this pt?	*/
  unsigned int pd_acc	: 1;		/* page table was accessed?	*/
  unsigned int pd_mbz	: 1;		/* must be zero			*/
  unsigned int pd_fmb	: 1;		/* four MB pages?		*/
  unsigned int pd_global: 1;		/* global (ignored)		*/
  unsigned int pd_avail : 3;		/* for programmer's use		*/
  unsigned int pd_base	: 20;		/* location of page table?	*/
} pd_t;

/* Structure for a page table entry */

typedef struct {

  unsigned int pt_pres	: 1;		/* page is present?		*/
  unsigned int pt_write : 1;		/* page is writable?		*/
  unsigned int pt_user	: 1;		/* is use level protection?	*/
  unsigned int pt_pwt	: 1;		/* write through for this page? */
  unsigned int pt_pcd	: 1;		/* cache disable for this page? */
  unsigned int pt_acc	: 1;		/* page was accessed?		*/
  unsigned int pt_dirty : 1;		/* page was written?		*/
  unsigned int pt_mbz	: 1;		/* must be zero			*/
  unsigned int pt_global: 1;		/* should be zero in 586	*/
  unsigned int pt_avail : 3;		/* for programmer's use		*/
  unsigned int pt_base	: 20;		/* location of page?		*/
} pt_t;

typedef struct{
  unsigned int pg_offset : 12;		/* page offset			*/
  unsigned int pt_offset : 10;		/* page table offset		*/
  unsigned int pd_offset : 10;		/* page directory offset	*/
} virt_addr_t;

typedef struct{
  int bs_status;			/* MAPPED or UNMAPPED or FREE or HEAPED*/
  int bs_pid;				/* process id using this slot   */
  int bs_vpno;				/* starting virtual page number */
  int bs_npages;			/* number of pages in the store */
  int bs_sem;				/* semaphore mechanism ?	*/
  struct bs_map_t *next;	/* handle multiple mappings */
  int ncount;
} bs_map_t;

typedef struct{
  int fr_status;			/* MAPPED or UNMAPPED		*/
  int fr_pid;				/* process id using this frame  */
  int fr_vpno;				/* corresponding virtual page no*/
  int fr_refcnt;			/* reference count		*/
  int fr_type;				/* FR_DIR, FR_TBL, FR_PAGE	*/
  int fr_dirty;
  int fr_use_counter;
}fr_map_t;


typedef struct scqueue{
	int	frno;		
	struct scqueue *next;	
	struct scqueue *prev;	
}scitem;

extern bs_map_t bsm_tab[];
extern fr_map_t frm_tab[];

/* Function prototype defs */
int getVictimFrame(int);
void scadd(int);
void scdelete(int);

int init_shared_pt();
virt_addr_t* mapToVirtAddr(unsigned long);
int initpolicy(int);
pd_t* load_pd(int);
pt_t* load_private_pt(pd_t*);
SYSCALL load_private_pg(pt_t*, unsigned long);
void WriteBackProcessData(int);

/* Prototypes for required API calls */
SYSCALL xmmap(int, bsd_t, int);
SYSCALL xunmap(int);
/* given calls for dealing with backing store */

int get_bs(bsd_t, unsigned int);
SYSCALL release_bs(bsd_t);
SYSCALL read_bs(char *, bsd_t, int);
SYSCALL write_bs(char *, bsd_t, int);

#define NBPG		4096	/* number of bytes per page	*/
#define FRAME0		1024	/* zero-th frame		*/
#define NFRAMES 	20	/* number of frames		*/

#define FREE	0
#define UNMAPPED 1
#define MAPPED	2
#define HEAPED	3

#define FR_PAGE		0
#define FR_TBL		1
#define FR_DIR		2

#define SC 3
#define LFU 4
#define DEFAULTPOLICY 3
#define BACKING_STORE_BASE	0x00800000
#define BACKING_STORE_UNIT_SIZE 0x00100000

/*added*/
#define BS_COUNT          16
#define BS_MAX_PAGES   128
#define	isbadbs(x)	(x<0 || x>=BS_COUNT)
#define isbadbspage(page)  ((page < 0) || (page >= BS_MAX_PAGES))

#define VPG_START          4096
#define NONE          -1	

#define	getpagefromaddr(x)	(x/NBPG)
#define	getaddrfrompage(x)	(x*NBPG)
#define getpagefromframe(x)	(FRAME0 + x)
#define getframefrompage(x)	(x - FRAME0)


#define	isbadvaddr(x)	((x/NBPG) < VPG_START)
#define	isbadframe(x)	(x<0 || x>=NFRAMES)
#define	isbadpagecount(x)	(x<=0 || x>BS_MAX_PAGES)

#define SHARED_PGT_COUNT	4
#define MAX_PG_PER_PT	1024
#define MAX_PT_PER_PD	1024

extern scitem* scqhead;
extern int debug;

