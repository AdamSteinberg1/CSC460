// Adam Steinberg
// CSC 460
// 02/28/2022
// Shared Bulletin Board

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct bbStruct
{
  int id;
  char name[20];
  int favNum;
  char favFood[30];
};


//returns a pointer to the shared memory of the Bulletin Board
struct bbStruct* attachToShMem()
{
  //read shared memory id from file
  FILE* fp;
  const char* path = "/pub/os/bb/BBID.txt";
  if((fp = fopen(path,"r")) == NULL)
  {
      printf("Error: Could not open \"%s\" to read.\n", path);
      exit(1);
  }

  int shmid;
  fscanf(fp, "%d", &shmid);
  fclose(fp);

  //attach to shared memory and return pointer
  struct bbStruct* shmem = shmat(shmid, NULL, SHM_RND);
  return shmem;
}

int main()
{
  struct bbStruct* bulletinBoard = attachToShMem();
  const int numRecords = 17;
  int i;
  for(i=0; i < numRecords; i++)
  {
    printf("%2d: %20s| %8d| %30s|\n", bulletinBoard[i].id, bulletinBoard[i].name, bulletinBoard[i].favNum, bulletinBoard[i].favFood);
  }
}
