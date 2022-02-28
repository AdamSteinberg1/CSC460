// Adam Steinberg
// Cheetah Synchronization
// CSC 460
// 03/04/2022

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
  //get input and error check
  if(argc != 2)
  {
    printf("Error: There must be exactly one argument\n");
    exit(1);
  }

  const int N = atoi(argv[1]);
  if(N < 1 || N > 26)
  {
    printf("Error: The argument %d is outside of the acceptable range [1,26].\n", N);
    exit(1);
  }

  //Ask OS for semaphores.
  const int sem_id = semget(IPC_PRIVATE, N, 0700);
  if (sem_id == -1)
  {
    printf("Error: Could not get semaphore.\n");
    exit(1);
  }

  //set first semaphore to 1
  semctl(sem_id, 0, SETVAL, 1);

  //set all other semaphores to 0
  int i;
  for(i=1; i<N; i++)
    semctl(sem_id, i, SETVAL, 0);

  int myID;
  char letter;

  //make N children
  for (i=0; i<N; i++)
  {
    if (fork()==0)
    {
      letter = 'A'+i;
      myID = i;
    }
    else
    {
      if(i==0)
        exit(0); //the original parent process dies here
      break;
    }
  }

  int lastPID=-1;
  if(i==N)
  {
    //get the PID of the last process
    lastPID = getpid();
  }


  for (i=0; i<N; i++)
  {
    p(myID, sem_id);

    //critical section
    //print my letter and PID
    printf("%c:%d\n", letter, getpid());

    v((myID+1)%N, sem_id);
  }

  //the last process is responsible for cleaning up semaphores
  if(lastPID == getpid())
  {
    if ((semctl(sem_id, 0, IPC_RMID, 0)) == -1)
    {
      printf("%s", "Error in removing sem\n");
    }
  }
}

p(int s, int sem_id)
{
  struct sembuf sops;

  sops.sem_num = s;
  sops.sem_op = -1;
  sops.sem_flg = 0;
  if((semop(sem_id, &sops, 1)) == -1) printf("%s", "'P' error\n");
}

v(int s, int sem_id)
{
  struct sembuf sops;

  sops.sem_num = s;
  sops.sem_op = 1;
  sops.sem_flg = 0;
  if((semop(sem_id, &sops, 1)) == -1) printf("%s","'V' error\n");
}
