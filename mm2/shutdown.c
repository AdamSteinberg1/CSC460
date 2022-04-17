// Adam Steinberg
// Memory Manager 1 - Shutdown
// CSC 460
// 04/18/2022

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

int main()
{   
    //read all the pids in death_row.dat and kill them
    FILE* fp = fopen("death_row.dat","rb");
    if(fp == NULL)
    {
        printf("Error: Could not open %s to read/write\n", "death_row.dat");
        exit(1);
    }

    int pid;
    while(fread(&pid, sizeof(int), 1, fp) == 1)
    {             
        if(pid>0) //a negative value means it's been deleted
        {
            kill(pid, SIGTERM);
        }
    }

    fclose(fp);
    remove("death_row.dat");
    return 0;
}
