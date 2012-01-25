#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <stdlib.h>
#include "util.h"

union semun {
	int val; 
	struct semid_ds *buf; 
	ushort *array;
};

int semCreate(int num){
	//int semid = semget(IPC_PRIVATE,num, IPC_CREAT|0666);
	int semid = semget(IPC_PRIVATE, num, 0644);
	if (semid == -1) {
		perror("Creating semaphors");
		return -1;
	}
	union semun semopts;
	unsigned short int sem_array[num]; /* gotta love VLAs */
	int i = 0;

	for (i =0 ; i< num; i++){
		sem_array[i] = 1;
	}
	semopts.array = sem_array;
	return (semctl(semid, 0, SETALL, semopts) == -1)? 
		(perror("Creating semaphors"),-1): semid;
}

int semWait(int semid, int semnum){
	struct sembuf sbuf;
	sbuf.sem_num = semnum;
	sbuf.sem_op = -1;
	sbuf.sem_flg = SEM_UNDO;
	return semop(semid, &sbuf, 1);
}

int semSignal(int semid, int semnum){
	struct sembuf sbuf;
	sbuf.sem_num = semnum;
	sbuf.sem_op = 1;
	sbuf.sem_flg = SEM_UNDO;
	return semop(semid, &sbuf, 1);
}

int semVal(int semid, int semnum){
	union semun smun;
	return semctl(semid, semnum, GETVAL, smun);
}

void semClose(int semgroupid){
	semctl(semgroupid,0,IPC_RMID,NULL);
}
