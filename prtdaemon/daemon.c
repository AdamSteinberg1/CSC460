#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define FILENAME "daemonData"
#define FRONT control_shmem[0]
#define REAR control_shmem[1]
#define MUTEX 0
#define FULL 1
#define EMPTY 2

struct printRequest
{
    int id;
    char filename[255];
};

//global variables
int sem_id;
int printQueue_shmid;
int control_shmid;
struct printRequest* printQueue;
int* control_shmem;

void cleanupSemaphores()
{
    if ((semctl(sem_id, 0, IPC_RMID, 0)) == -1)
        printf("%s", "Error in removing semaphores.\n");
}

void cleanupPrintQueueShmem()
{
    if ((shmctl(printQueue_shmid, IPC_RMID, NULL)) == -1)
        printf("Error removing printQueue shared memory.\n");
}

void cleanupControlShmem()
{
    if ((shmctl(control_shmid, IPC_RMID, NULL)) == -1)
        printf("Error removing control shared memory.\n");
}

void cleanupFile()
{
    if(remove(FILENAME) != 0)
        printf("Error removing file \"%s\"\n", FILENAME);
}

void setup(int printQueueSize) 
{
    
    //get three semaphores
    sem_id = semget (IPC_PRIVATE, 3, 0777);
    if (sem_id == -1)
    {
        printf("Error: semget failed.\n");
        exit(1);
    }
    atexit(cleanupSemaphores);

    //initialize the semaphores
    semctl(sem_id, MUTEX, SETVAL, 1);   //mutex starts unlocked
    semctl(sem_id, FULL, SETVAL, 0);    //there are 0 full slots
    semctl(sem_id, EMPTY, SETVAL, printQueueSize);    //there are printQueueSize empty slots

    //get shared memory for the printQueue
    printQueue_shmid = shmget(IPC_PRIVATE, printQueueSize*sizeof(struct printRequest), 0700);
    if (printQueue_shmid == -1)
    {
        printf("Error: shmget failed for printQueue.\n");
        exit(1);
    }
    atexit(cleanupPrintQueueShmem);
    printQueue = (struct printRequest *) shmat(printQueue_shmid, NULL, SHM_RND);
    

    //get shared memory for control memory
    //i.e. ints for where the front and rear of the print queue is
    control_shmid = shmget(IPC_PRIVATE, 2*sizeof(int), 0700);
    if (control_shmid == -1)
    {
        printf("Error: shmget failed for control memory.\n");
        exit(1);
    }
    atexit(cleanupControlShmem);
    control_shmem = (int *) shmat(control_shmid, NULL, SHM_RND);

    //initialize control shared memory
    FRONT = 0;
    REAR = 0;

    //write data that must be shared to the users in a file
    FILE* fp = fopen(FILENAME,"w");
    if(fp == NULL)
    {
        printf("Error: Could not open %s to write\n", FILENAME);
    }
    fprintf(fp, "%d\n%d\n%d\n%d\n%d\n", getpid(), printQueue_shmid, control_shmid, sem_id, printQueueSize);
    fclose(fp);
    atexit(cleanupFile);
}

void printFile(char* filename)
{
    char command[259] = "cat ";
    strcat(command, filename);
    system(command);
}

p(int s,int sem_id)
{
    struct sembuf sops;

    sops.sem_num = s;
    sops.sem_op = -1;
    sops.sem_flg = 0;
    if((semop(sem_id, &sops, 1)) == -1)
        printf("%s", "'P' error\n");
}

v(int s, int sem_id)
{
    struct sembuf sops;

    sops.sem_num = s;
    sops.sem_op = 1;
    sops.sem_flg = 0;
    if((semop(sem_id, &sops, 1)) == -1) 
        printf("%s","'V' error\n");
}

void promptForNumber(int* printQueueSize)
{
    do
    {
        printf("Error: %d outside of acceptable range [1,10]\n", *printQueueSize);
        printf("Please try again (-1 to quit): ");
        scanf("%d", printQueueSize);
        if(*printQueueSize == -1)
            exit(1);
    } while (*printQueueSize < 1 || *printQueueSize > 10);
}

int main(int argc, char const *argv[])
{   
    //if this program recieves a SIGTERM, exit gracefully by calling all atexit() callbacks
    signal(SIGTERM, exit);

    //error check the command line args
    if(argc != 2) 
    {
        printf("Error: There must be exactly one command line argument\n");
        exit(1);
    }

    int printQueueSize = atoi(argv[1]);
    if(printQueueSize < 1 || printQueueSize > 10) 
    {
        promptForNumber(&printQueueSize);
    }

    //check if the daemon is already running
    if(access(FILENAME, F_OK) == 0)
    {
        printf("Error: The daemon is already running.\n");
        exit(1);
    }

    //setup shared resources
    setup(printQueueSize);

    //process the queue until program is killed
    for(;;) //infinite loop
    {
        p(FULL, sem_id);
        p(MUTEX, sem_id);

        //Critical Section
        struct printRequest currRequest = printQueue[FRONT];
        printf("User %d's file is being printed.\n", currRequest.id);
        printFile(currRequest.filename);
        FRONT = (FRONT+1)%printQueueSize;
        
        v(MUTEX, sem_id);
        v(EMPTY, sem_id);
    }
}

