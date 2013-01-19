#define _GNU_SOURCE 1
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <stdlib.h>
#include "util.h"

int shmCreate(int size){
	return shmget(IPC_PRIVATE, size, IPC_CREAT|0666);
}

int shmDelete(int id){
	return shmctl(id, IPC_RMID, NULL);
}

unsigned long getShmMin(){
  struct shminfo s;
  if (shmctl(0, IPC_INFO, (struct shmid_ds *)&s) <= 0)  return 0;
  return  s.shmmin;
}

unsigned long getShmMax(){
  struct shminfo s;
  if (shmctl(0, IPC_INFO, (struct shmid_ds *)&s) <= 0)  return 0;
  return  s.shmmax;
}
