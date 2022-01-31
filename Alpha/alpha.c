// Adam Steinberg
// CSC 460
// 02/04/2022
// Alpha^Alpha
#include <stdio.h>

//a function that should waste some CPU cycles
void wasteTime()
{
    const int N = 5000;
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

int main(int argc, char const *argv[])
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

    int i;
    char currChar ='A';
    for(i=0; i < N; i++)
    {
        if(fork()==0)
        {
            int j;
            for(j=0; j < N; j++)
            {
                printf("%c:%d\n", currChar, getpid());
                wasteTime();
            }
            break;
        }
        currChar++;
    }
}

