#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define DATAFILE "daemonData"
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
int printQueueSize;

//stores a "Quote of the Day" into the string that quote points to 
//returns the length of the quote
ssize_t getQuote(char** quote)
{
    FILE* fp = popen("curl -s http://api.quotable.io/random | cut -d\":\" -f4 | cut -d\"\\\"\" -f2", "r");
    size_t bufferSize  = 0;
    getline(quote, &bufferSize, fp);
    pclose(fp);   
}

//makes a file with the quote of the day
void makeFile(char* filename)
{   
    //open the file to write
    FILE *fp = fopen(filename, "w");
    if(fp == NULL)
    {
        printf("Error: could not open %s to write.", filename);
        exit(1);
    }

    //write the quote
    char* quote;
    getQuote(&quote);
    fputs(quote, fp);

    fclose(fp);
}

void readDataFile()
{
    //read data from shared file
    FILE* fp = fopen(DATAFILE,"r");
    if(fp == NULL)
    {
        printf("Error: Could not open %s to read. The daemon is not running.\n", DATAFILE);
        exit(1);
    }
    int daemonPid; //the user doesn't need the daemonPid, so just read in and ignore
    fscanf(fp, "%d\n%d\n%d\n%d\n%d\n", &daemonPid, &printQueue_shmid, &control_shmid, &sem_id, &printQueueSize);
    fclose(fp);
}

void setup() 
{
    readDataFile();

    //get shared memory for the printQueue
    printQueue = (struct printRequest *) shmat(printQueue_shmid, NULL, SHM_RND);

    //get shared memory for control memory
    //i.e. ints for where the front and rear of the print queue is
    control_shmem = (int *) shmat(control_shmid, NULL, SHM_RND);
}

//simulates working for a random amount of time
void work()
{
    int duration = rand()%3 + 1;
    printf("User %d is working for %d seconds.\n", getpid(), duration);
    sleep(duration);
}

//puts a print request in the print queue
void requestPrint(char* filename)
{
    struct printRequest currRequest;
    currRequest.id = getpid();
    strcpy(currRequest.filename, filename);

    p(EMPTY, sem_id);
    p(MUTEX, sem_id);

    //Critical Section
    printf("User %d is printing %s.\n", getpid(), filename);
    printQueue[REAR] = currRequest;
    REAR = (REAR+1)%printQueueSize;


    v(MUTEX,sem_id);
    v(FULL,sem_id);
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

//checks if the daemon is alive (i.e. stop has not been run)
//exits the program if not
void checkDaemon()
{
    if(access(DATAFILE, F_OK) != 0)
    {
        printf("Error: User %d wants to print, but the daemon is not running\n", getpid());
        printf("User %d is logging off\n", getpid());
        exit(1);
    }
}

int main(int argc, char const *argv[])
{
    //the file name has the form "file#####" where #### is the pid
    char filename[20];
    sprintf(filename, "%s%d", "file", getpid());

    //make the file to be printed
    makeFile(filename);
    
    //set up shared resources
    setup();

    //seed random number generator
    srand(getpid());

    printf("User %d is logging on\n", getpid());

    int i;
    for(i=0; i<5; i++)
    {
        work();
        checkDaemon();
        requestPrint(filename);
    }

    printf("User %d is logging off\n", getpid());
}
