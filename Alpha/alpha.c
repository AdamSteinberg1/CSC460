// Adam Steinberg
// CSC 460
// 02/04/2022
// Alpha^Alpha
#include <stdio.h>
#include <stdlib.h>

//a function that should waste some CPU cycles
void wasteTime()
{
    const int N = 2000;
    int i;
    for(i=0; i<N; i++)
    {
        int j;
        for(j=0; j<N; j++)
        {
            i*j;
        }
    }
}

//prints a letter N times and then exits
void printLetter(char letter, int N)
{
  int i;
  for(i=0; i < N; i++)
  {
      printf("%c:%d\n", letter, getpid());
      wasteTime();
  }
  exit(0);
}

int main(int argc, char const *argv[])
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


    char letter;
    for(letter='A'; letter < 'A'+N; letter++) //iterate through N letters
        if(fork()==0) //make a child process to print it's letter
          printLetter(letter, N);
}
