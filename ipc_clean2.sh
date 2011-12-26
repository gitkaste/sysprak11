#!/bin/bash
################################################################
# remove all shared memory segments with no associated process #
################################################################

# remove the first lines of ipcrs -m
X=$(ipcs -m|wc -l)
IPCS=$(ipcs -m|tail -n $((X-3))|tr -s " " _)
echo -e "\nlooking for unused shared memory segments"
echo "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^"
# check, if nattch is 0 and remove the shared memory segment
for A in $IPCS
  do
  NATTCH=$(echo $A|awk -F _ '{print $6}')
  ID=$(echo $A|awk -F _ '{print $2}')
  if (($NATTCH==0)); then
      echo -e "\tremoving shm id $ID with $NATTCH associated processes"
      ipcrm -m $ID #>&/dev/null # <- Ausgabe nach /dev/null umleiten
  fi
done
echo -e "done\n"

################################
# remove all(!) semaphore sets #
################################

# remove the first lines of ipcrs -s
X=$(ipcs -s|wc -l)
IPCS=$(ipcs -s|tail -n $((X-3))|tr -s " " _)
echo "looking for semaphore sets"
echo "^^^^^^^^^^^^^^^^^^^^^^^^^^"
# remove all semaphore sets
for A in $IPCS
  do
  ID=$(echo $A|awk -F _ '{print $2}')
  echo -e "\tremoving semaphor with id $ID"
  ipcrm -s $ID #>&/dev/null # <- Ausgabe nach /dev/null umleiten
done
echo -e "done\n"
