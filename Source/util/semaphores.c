#include "util_sem_defines.h"
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <stdlib.h>

union semun {
	int val; 
	struct semid_ds *buf; 
	ushort *array;
};

int semCreate(int num){
	int semid = semget(IPC_PRIVATE,num,0);
	if (semid == -1) {
		//puts("Problem creating semph. group id");
		return -1;
	}
	union semun smun;
	smun.val=1;
	return (semctl(semid, 2, SETALL, smun) == -1)? -1: semid;
}

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
	union semun smun;
	return semctl(semid, semnum, GETVAL, smun);
}

void semClose(int semgroupid){
	semctl(semgroupid,0,IPC_RMID,NULL);
}
