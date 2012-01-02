#include "util_sem_defines.h"
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

int semWait(int semid, int semnum){
	struct sembuf sbuf;
	sbuf.semnum = semnum;
	sbuf.sem_op = -1;
	return semop(semid, &sbuf, 1);
}
int semSignal(int semid, int semnum){
	struct sembuf sbuf;
	sbuf.semnum = semnum;
	sbuf.sem_op = 1;
	return semop(semid, &sbuf, 1);
}
int semVal(int semid, int semnum){
	union semun smun;
	return semctl(semid, semnum, GETVAL,smun);
}
