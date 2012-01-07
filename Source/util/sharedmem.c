#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "util.h"

int shmCreate(int size){
	return shmget(IPC_KEY, size, IPC_CREAT|0666);
}

int shmDelete(int id){
	return shmctl(id, IPC_RMID, NULL);
}

