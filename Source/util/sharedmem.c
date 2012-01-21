#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdlib.h>
#include "util.h"

int shmCreate(int size){
	return shmget(IPC_KEY, size, IPC_CREAT|0666);
}

int shmDelete(int id){
	return shmctl(id, IPC_RMID, NULL);
}

int getShmMin(){
	int size;
	FILE * f = fopen ("/proc/sys/kernel/shmmin", "r");
	if (!f) return -1;
	if (!fscanf(f, "%d", &size)) {
		fclose(f);
		return -1;
	}
	return size;
}

int getShmMax(){
	int size;
	FILE * f = fopen ("/proc/sys/kernel/shmmax", "r");
	if (!f) return -1;
	if (!fscanf(f, "%d", &size)) {
		fclose(f);
		return -1;
	}
	return size;
}
