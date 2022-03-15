#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#define FILENAME "daemonData"

//gets the pid of the daemon
int readPid()
{
    FILE* fp = fopen(FILENAME, "r");
    if(fp == NULL)
    {
        printf("Error: Could not open %s to read\n"
                "Has the daemon already been killed?\n", 
                FILENAME);
        exit(1);
    }
    int pid;
    fscanf(fp, "%d", &pid);
    fclose(fp);
    return pid;
}

int main(int argc, char const *argv[])
{   
    //get the pid of the daemon process
    int pid = readPid();
    //kill it! 
    printf("Killing the daemon! 😈\n");
    kill(pid, SIGTERM);
}
