#define RANDOMSCHED 1
#define LINUXSCHED 2
extern int globalschedclass;
extern void setschedclass(int sched_class);
extern int getschedclass();
extern int epoch;
