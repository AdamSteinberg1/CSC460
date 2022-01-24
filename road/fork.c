// Adam Steinberg
// CSC 460
// 01/31/2022
// Fork in the Road
#include <stdio.h>

//spawns the specified number of children processes
//prints a row in the table if the process is a parent
void spawnChildren(int numChildren, int treeDepth)
{
  if(numChildren < 0)
    return;

  int childPID = -1;
  int i;
  for(i=0; i<numChildren; i++)
  //only make a child process, if the current process is not itself a child
    if(childPID != 0)
      childPID = fork();

  //treeDepth remains constant throughout the recursion
  //numChildren is decremented at each recursion level
  if(childPID == 0)
    spawnChildren(numChildren-1, treeDepth);
  else
    printf("%d\t%d\t%d\n", treeDepth-numChildren, getpid(), getppid());
}

int main(int argc, char const *argv[])
{
  //get input and error check
  if(argc != 2)
  {
    printf("Error: There must be exactly one argument\n");
    return 1;
  }

  int treeDepth = atoi(argv[1]);
  if(treeDepth < 0 || treeDepth > 5)
  {
    printf("Error: The argument %d is outside of the acceptable range [0,5].\n", treeDepth);
    return 1;
  }


  printf("Gen\tPID\tPPID\n");           //print table header
  spawnChildren(treeDepth, treeDepth);  //spawns
  sleep(1);
}
