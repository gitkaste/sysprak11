/* Semaphore defines */
/* General */
#define SEM_LOGGER 0
/* Server */
#define SEM_FILELIST 1
/* Client */
#define SEM_CONSOLER 1
#define SEM_RESULTS 2
int semCreate(int num);
int semWait(int semid, int semnum);
int semSignal(int semid, int semnum);
int semVal(int semid, int semnum);
void semClose(int semgroupid);
