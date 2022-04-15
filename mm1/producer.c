// Adam Steinberg
// Memory Manager 1 - Producer
// CSC 460
// 04/18/2022

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

#define FILENAME "ipc.dat"
#define REAR buffer_control[1]
#define MUTEX 0
#define FULL 1
#define EMPTY 2

//what the bounded buffer stores
struct jobRequest
{
    int pid, size, duration, sem_id;
};

//global variables
struct jobRequest* buffer = NULL;
int* buffer_control = NULL;
int buffer_id = -1;
int sem_id = -1;
int buffer_size = -1;
int RAM_size = -1;
int job_sem_id = -1;


//death_row.dat is a file that contains all pids of processes that should be killed by shutdown.c
void writeToDeathRow(int pid)
{
    FILE* fp = fopen("death_row.dat","a+b");
    if(fp == NULL)
    {
        printf("Error: Could not open %s to read/write\n", "death_row.dat");
    }


    fseek(fp, 0, SEEK_SET); //seek to beginning of file


    //read ints from file until EOF or encounter deleted pid
    int readVal;
    while(fread(&readVal, sizeof(int), 1, fp) == 1)
    {
        if (readVal < 0) //negative value is a marker for deleted pid
        {
            fseek(fp, -1*sizeof(int), SEEK_CUR); //back up one integer so rw head is on the integer we just read
            break;      
        }  
    }

    fwrite(&pid, sizeof(int), 1, fp); //write new pid, either at eof or on top of deleted pid

    fclose(fp);
}

//deletes pid from death_row.dat
void deleteFromDeathRow(int pid)
{
    FILE* fp = fopen("death_row.dat","a+b");
    if(fp == NULL)
    {
        printf("Error: Could not open %s to read/write\n", "death_row.dat");
    }


    fseek(fp, 0, SEEK_SET); //seek to beginning of file


    //read ints from file until EOF or encounter deleted pid
    int readVal;
    while(fread(&readVal, sizeof(int), 1, fp) == 1)
    {
        if (readVal == pid) //negative value is a marker for deleted pid
        {
            fseek(fp, -1*sizeof(int), SEEK_CUR); //back up one integer so rw head is on the integer we just read
            int negativeOne = -1; //a negative value means it's deleted
            fwrite(&negativeOne, sizeof(int), 1, fp);
            break;      
        }  
    }
    fclose(fp);
}

void readFile()
{
    //read data from shared file
    FILE* fp = fopen(FILENAME,"r");
    if(fp == NULL)
    {
        printf("Error: Could not open %s to read.\n", FILENAME);
        exit(1);
    }
    fscanf(fp, "%d\n%d\n%d\n%d", &sem_id, &buffer_id, &buffer_size, &RAM_size);
    fclose(fp);
}

void setup() 
{
    readFile();

    //get shared memory for the buff
    buffer = (struct jobRequest *) shmat(buffer_id, NULL, SHM_RND);
    buffer_control = (int*) (buffer+buffer_size);
}

//enqueues a job request in the buffer
void requestJob(int size, int duration, int job_sem_id)
{
    struct jobRequest currRequest;
    currRequest.pid = getpid();
    currRequest.duration = duration;
    currRequest.size = size;
    currRequest.sem_id = job_sem_id;

    p(EMPTY, sem_id);
    p(MUTEX, sem_id);

    //Critical Section
    printf("Producer %d is requesting %d blocks of RAM for %d seconds.\n", getpid(), size, duration);
    buffer[REAR] = currRequest;
    REAR = (REAR+1)%buffer_size;

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

bool isNum(const char* s)
{
    const int n = strlen(s);
    int i;
    for(i=0; i < n; i++)
    {
        if (!isdigit(s[i]))
        {
            return false;
        }
    }
    return true;
}

//removes the job semaphore
void cleanup()
{
    if(job_sem_id != -1)
        if (semctl(job_sem_id, 0, IPC_RMID, 0) == -1)
            printf("%s", "Error removing semaphore.\n");
}

//takes in argc and argv
//returns through the arguments the values of the argv
//exits on errors
void parseArgs(int argc, char const *argv[], int* size, int* duration)
{
    if(argc != 3)
    {
        printf("Error: There must be exactly 2 arguments.\n");
        exit(1);
    }

    int i;
    for(i=1; i<argc; i++)
    {
        if(!isNum(argv[i]))
        {
            printf("Error: %s is not an integer.\n", argv[i]);
            exit(1);
        }
    }

    *size = atoi(argv[1]);
    if(*size > RAM_size)
    {
        printf("Error: The size must not exceed RAM's capacity\n");
        exit(1);
    }
    if(*size < 1)
    {
        printf("Error: The size must be greater than 0\n");
        exit(1);
    }

    *duration = atoi(argv[2]);
    if(*duration < 1 || *duration > 30)
    {
        printf("Error: The duration must be in the range [1,30] \n");
        exit(1);
    }
}

int makeSemaphore()
{
    //get a semaphore
    int job_sem_id = semget(IPC_PRIVATE, 1, 0700);
    if (job_sem_id == -1)
    {
        printf("Error: semget failed.\n");
        exit(1);
    }

    //initialize the semaphore to 0
    semctl(job_sem_id, 0, SETVAL, 0);
    return job_sem_id;
}

int main(int argc, char const *argv[])
{
    //gracefully exit when killed
    //catch multiple types of kill signals
    signal(SIGTERM, exit);
    signal(SIGINT, exit);
    signal(SIGQUIT, exit);
    signal(SIGTSTP, exit);
    atexit(cleanup);

    writeToDeathRow(getpid());
    setup(); //read file and attach to shared resources

    int size, duration;
    parseArgs(argc, argv, &size, &duration);
    job_sem_id = makeSemaphore(); //get a semaphore to send with this job

    requestJob(size, duration, job_sem_id);

    p(0, job_sem_id); //block until job is finished

    printf("Producer %d finished it's request of %d blocks for %d seconds.\n", getpid(), size, duration);

    deleteFromDeathRow(getpid());
}
