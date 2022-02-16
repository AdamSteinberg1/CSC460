// Adam Steinberg
// Tortoise Synchronization
// CSC 460
// 02/21/2022

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
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

  //allocate shared memory and get id
  const int shmid  =  shmget(IPC_PRIVATE, sizeof(int), 0600);
  if (shmid == -1)
  {
    printf("Could not get shared memory.\n");
    exit(1);
  }

  //attach to shared memory
  int* shmem = (int *) shmat(shmid, NULL, SHM_RND);

  //the first ID is 0
  *shmem = 0;

  int myID;
  char letter;

  //make N children
  int i;
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
    //check the shared memory until it has myID in it
    while (*shmem != myID);

    //print my letter and PID
    printf("%c:%d\n", letter, getpid());

    //update the ID in shared memory
    *shmem = (myID+1)%N;
  }

  //the last process is responsible for cleaning up shared memory
  if(lastPID == getpid())
  {
    if ((shmctl(shmid, IPC_RMID, NULL)) == -1)
    {
      printf("ERROR removing shmem.\n");
    }
  }
}
