#include "util_sem_defines.h"
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

int semWait(int semid, int semnum){
	struct sembuf sbuf;
	sbuf.sem_num = semnum;
	sbuf.sem_op = -1;
	return semop(semid, &sbuf, 1);
}
int semSignal(int semid, int semnum){
	struct sembuf sbuf;
	sbuf.sem_num = semnum;
	sbuf.sem_op = 1;
	return semop(semid, &sbuf, 1);
}
int semVal(int semid, int semnum){
	union semun {
		int val; 
		struct semid_ds *buf; 
		ushort *array;
	} smun;
	return semctl(semid, semnum, GETVAL, smun);
}
