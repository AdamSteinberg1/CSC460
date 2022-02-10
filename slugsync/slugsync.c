// Adam Steinberg
// Sluggish Synchronization
// CSC 460
// 02/14/2022

#include <stdio.h>

int main(int argc, char *argv[])
{
  //get input and error check
  if(argc != 2)
  {
      printf("Error: There must be exactly one argument\n");
      return 1;
  }

  const int N = atoi(argv[1]);
  if(N < 1 || N > 26)
  {
      printf("Error: The argument %d is outside of the acceptable range [1,26].\n", N);
      return 1;
  }

  FILE *fp;
  if((fp = fopen("syncfile","w")) == NULL)
  {
      printf("Error: Could not open syncfile to write.\n");
      return 1;
  }

  fprintf(fp,"%d",0); //ID 0 is the first to execute
  fclose(fp);

  int myID=N-1; //this will be the ID of the parent process, which handles the last letter
  char letter = 'A';

  //make N-1 children
  //the parent process handles the Nth letter
  int i;
  for (i=0; i<N-1; i++)
  {
      if (fork() == 0)
      {
          myID = i;
          break;
      }
      letter++;
  }

  for (i=0; i<N; i++)
  {
      int found = -1;
      //check the file until it has myID in it
      while (found != myID)
      {
          if((fp = fopen("syncfile","r")) == NULL)
          {
              printf("Error: Could not open syncfile to read.\n");
              return 1;
          }

          fscanf(fp,"%d",&found);
          fclose(fp);
      }

      //print my letter and PID
      printf("%c:%d\n", letter, getpid());

      //update the file with the next ID
      if((fp = fopen("syncfile","w")) == NULL)
      {
          printf("Error: Could not open syncfile to write.\n");
          return 1;
      }
      fprintf(fp,"%d", (myID+1)%N);
      fclose(fp);
  }
}
