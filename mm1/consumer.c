// Adam Steinberg
// Memory Manager 1 - Consumer
// CSC 460
// 04/18/2022

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/queue.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#define FILENAME "ipc.dat"
#define MUTEX 0
#define FULL 1
#define EMPTY 2
#define MAX_JOBS 100 //the maximum number of jobs the system can handle at once

//what the bounded buffer stores
struct jobRequest
{
    int pid, size, duration, sem_id;
};

struct Job
{
    char id;
    int pid, size, durationRemaining, sem_id, location;
};

struct ProcessControlBlock
{
    int job_count;
    int RAM_size;
    struct Job jobs[MAX_JOBS];
};

struct buffer_data
{
    struct jobRequest* buffer;
    int* front;
    int* rear;
    int buffer_id;
    int sem_id;
};

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

//returns the next character that should be used for the ID
char nextCharID()
{
    static int counter = 0;
    char id = 'A'+counter;
    counter = (counter+1)%26;
    return id;
}

//returns true if s is a number
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

//removes all shared resources fro the buffer, if they exist
void cleanupBuffer(int status, void* args)
{
    int* args_array = (int*)args;
    int sem_id = args_array[0];
    int buffer_id = args_array[1];
    if(sem_id != -1)
        if (semctl(sem_id, 0, IPC_RMID, 0) == -1)
            printf("%s", "Error removing semaphores.\n");

    if(buffer_id != -1)
        if (shmctl(buffer_id, IPC_RMID, NULL) == -1)
            printf("Error removing buffer shared memory.\n");

    if(access(FILENAME, F_OK) == 0)        
        if(remove(FILENAME) != 0)
            printf("Error removing file \"%s\"\n", FILENAME);
}

//takes in argc and argv
//returns through the arguments the values of the argv
//exits on errors
void parseArgs(int argc, char const *argv[], int* row_count, int* col_count, int* buffer_size)
{
    if(argc != 4)
    {
        printf("Error: There must be exactly 3 arguments.\n");
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

    *row_count = atoi(argv[1]);
    if(*row_count < 1 || *row_count > 20)
    {
        printf("Error: The number of rows must be in the range [1,20] \n");
        exit(1);
    }

    *col_count = atoi(argv[2]);
    if(*col_count < 1 || *col_count > 50)
    {
        printf("Error: The number of columns must be in the range [1,50] \n");
        exit(1);
    }
    
    *buffer_size = atoi(argv[3]);
    if(*buffer_size < 1 || *buffer_size > 26)
    {
        printf("Error: The buffer size must be in the range [1,26] \n");
        exit(1);
    }
}

struct buffer_data setupBuffer(int buffer_size, int RAM_size) 
{
    //get three semaphores
    const int sem_id = semget(IPC_PRIVATE, 3, 0700);
    if (sem_id == -1)
    {
        printf("Error: semget failed.\n");
        exit(1);
    }

    //initialize the semaphores
    semctl(sem_id, MUTEX, SETVAL, 1);   //mutex starts unlocked
    semctl(sem_id, FULL, SETVAL, 0);    //there are 0 full slots
    semctl(sem_id, EMPTY, SETVAL, buffer_size);    //there are buffer_size empty slots

    //get shared memory for the buffer
    const int buffer_id = shmget(IPC_PRIVATE, buffer_size*sizeof(struct jobRequest) + 2*sizeof(int), 0700);
    if (buffer_id == -1)
    {
        printf("Error: shmget failed for buffer.\n");
        exit(1);
    }
    static int args[2];
    args[0] = sem_id;
    args[1] = buffer_id;
    on_exit(cleanupBuffer, args);

    struct jobRequest* buffer = (struct jobRequest *) shmat(buffer_id, NULL, SHM_RND);
    int* front = (int*) (buffer+buffer_size);
    int* rear = front+1;

    //initialize front and rear pointers
    *front = 0;
    *rear = 0;

    //write data that must be shared to the users in a file
    FILE* fp = fopen(FILENAME,"w");
    if(fp == NULL)
    {
        printf("Error: Could not open %s to write\n", FILENAME);
    }
    fprintf(fp, "%d\n%d\n%d\n%d", sem_id, buffer_id, buffer_size, RAM_size);
    fclose(fp);

    struct buffer_data result = {buffer, front, rear, buffer_id, sem_id};
    return result;
}

//fills RAM based on the information from the process control block
void fillRAM(char* RAM, struct ProcessControlBlock* pcb)
{
    int i;
    for(i=0; i<pcb->RAM_size; i++)
        RAM[i] = '.';

    for(i=0; i<MAX_JOBS; i++)
    {
        struct Job* job = &(pcb->jobs[i]);
        if(job->pid < 0) //ignore removed jobs
            continue;
        int start = job->location;
        if(start < 0) //ignore jobs not in RAM
            continue;
        int end = start + job->size;

        int j;
        for(j=start; j<end; j++)
        {
            if(RAM[j] != '.') //TODO remove after debugging
                printf("Error: overwriting process in RAM at location %d\n", j);
            RAM[j]=job->id;
        }
    }
}

//returns true if a job of size jobSize can fit at location in RAM
bool fit(int jobSize, int location, char* RAM, int RAM_size)
{
    int i;
    for(i=0; i<jobSize; i++)
    {   
        if(location+i >= RAM_size)
        {
            return false;
        }
        if(RAM[location+i] != '.')
        {
            return false;
        }
    }
    return true;
}

//places a job into RAM using first fit
//returns true if it was successful at placing job
bool placeIntoRAM(struct Job* job, char* RAM, int RAM_size)
{
    int i;
    for(i=0; i<RAM_size; i++) //iterate through all RAM locations
    {
        if(fit(job->size, i, RAM, RAM_size)) //if a job fits starting at location i
        {
            //fill RAM with job's id
            int j;
            for(j=0; j<job->size; j++)
                RAM[i+j] = job->id;

            job->location = i; //update job's location
            return true;
        }
    }
    return false;
}

//adds a job to the Process Control Block
void addJob(struct ProcessControlBlock* pcb, struct Job job)
{
    if(pcb->job_count >= MAX_JOBS)
    {
        printf("Fatal Error: The Process Control Block has been completely filled.\n");
        system("./shutdown");
        exit(1);
    }

    static int rear = 0;
    int i;
    for(i=0; i<MAX_JOBS; i++)
    {   
        if(pcb->jobs[rear].pid < 0)
        {
            pcb->jobs[rear] = job;
            break;
        }
        rear = (rear+1)%MAX_JOBS;
    }
    pcb->job_count++;
}

void consumeBuffer(int buffer_size, struct buffer_data data, struct ProcessControlBlock* pcb)
{ 
    while(true) //infinite loop
    {
        p(FULL, data.sem_id);
        p(MUTEX, data.sem_id);

        //Critical Section
        struct jobRequest request = data.buffer[*(data.front)];
        struct Job job;
        job.durationRemaining = request.duration;
        job.pid = request.pid;
        job.sem_id = request.sem_id;
        job.size = request.size;
        job.location = -1;
        job.id = nextCharID();
        addJob(pcb, job);
        *(data.front) = (*(data.front)+1)%buffer_size;

        v(MUTEX, data.sem_id);
        v(EMPTY, data.sem_id);
    }
}

void spawnBufferConsumer(int buffer_size, int RAM_size, int pcb_id)
{
    if(fork() == 0)
    {   
        writeToDeathRow(getpid());
        struct buffer_data data = setupBuffer(buffer_size, RAM_size);
        struct ProcessControlBlock* pcb = (struct ProcessControlBlock *) shmat(pcb_id, NULL, SHM_RND);
        consumeBuffer(buffer_size, data, pcb);
        exit(1);
    }
}

//removes shared memory
void cleanupProcessControlBlock(int status, void* args)
{
    int id = *((int*)args);

    if (shmctl(id, IPC_RMID, NULL) == -1)
        printf("Error removing jobs shared memory.\n");
}

//gets shared memory for process control block and initializes it
//returns shared memory id to the process control block
int setupProcessControlBlock(int RAM_size)
{   
    //get shared memory
    static int id;
    id = shmget(IPC_PRIVATE, sizeof(struct ProcessControlBlock), 0700);
    if (id == -1)
    {
        printf("Error: shmget failed for Process Control Block.\n");
        exit(1);
    }

    struct ProcessControlBlock* pcb = (struct ProcessControlBlock *) shmat(id, NULL, SHM_RND);
    pcb->job_count=0;
    pcb->RAM_size=RAM_size;

    //initialize all job pids to -1, to represent an empty slot in the array
    int i;
    for(i=0; i<MAX_JOBS; i++)
    {
        pcb->jobs[i].pid = -1;
    }

    if (shmdt(pcb) == -1) 
        printf("Error detaching from shared memory.\n");

    return id;
}

void displayJobs(struct ProcessControlBlock* pcb)
{
    printf("%-2s %-6s %-4s %s\n","ID", "PID", "Size", "Sec");
    int i;
    for(i=0; i<MAX_JOBS; i++)
    {
        struct Job* job = &(pcb->jobs[i]);
        if(job->pid < 0) //don't print removed jobs
            continue;
        printf("%-2c %-6d %-4d %d\n", job->id, job->pid, job->size, job->durationRemaining);
    }
}

void displayRAM(char* RAM, int row_count, int col_count)
{
    int i;

    //print top border
    printf("┏");
    for(i=0; i<col_count; i++)
        printf("━");
    printf("┓");

    putchar('\n');
    for(i=0; i<row_count; i++)
    {
        printf("┃"); //print left border
        int j;
        for(j=0; j<col_count; j++)
        {   
            putchar(RAM[i*col_count+j]);
        }
        printf("┃"); //print right border
        putchar('\n');
    }

    //print bottom border
    printf("┗");
    for(i=0; i<col_count; i++)
        printf("━");
    printf("┛");
}

void display(struct ProcessControlBlock* pcb, char* RAM, int row_count, int col_count)
{   
    printf("\n");
    displayJobs(pcb);
    printf("\n");
    displayRAM(RAM, row_count, col_count);
    printf("\n");
}

void decrementDurations(struct ProcessControlBlock* pcb)
{
    int i;
    for(i=0; i<MAX_JOBS; i++)
    {
        struct Job* job = &(pcb->jobs[i]);
        if(job->pid < 0) //don't decrement a removed job
            continue;
        if(job->location < 0)  //don't decrement a job not in RAM
            continue;
        job->durationRemaining--;
    }
}

void removeJobs(struct ProcessControlBlock* pcb, char* RAM)
{
    int i;
    for(i=0; i<MAX_JOBS; i++)
    {
        struct Job* job = &(pcb->jobs[i]);
        if(job->durationRemaining <= 0 && job->pid >= 0) 
        {
            job->pid = -1; //a negative pid indicates the job has been removed
            pcb->job_count--;

            //clear out the job's ids from RAM
            int j;
            for(j=0; j<job->size; j++)
                RAM[j+job->location] = '.';

            v(0, job->sem_id); //wake up the producer process
        }
    }
}

void placeJobsIntoRAM(struct ProcessControlBlock* pcb, char* RAM)
{   
    static int front = 0;
    const int start = front;
    int i;
    for(i=0; i<MAX_JOBS; i++)
    {
        struct Job* job = &(pcb->jobs[(start+i)%MAX_JOBS]);
        if(job->pid < 0) //don't place a removed job in RAM
            continue;
        if(job->location >= 0)  //don't place a job in RAM if it's already there
            continue;
        if(placeIntoRAM(job, RAM, pcb->RAM_size) && i == front)
            front=(front+1)%MAX_JOBS;
    }
}

void manageMemory(int* pcb_id, int row_count, int col_count)
{
    struct ProcessControlBlock* pcb = (struct ProcessControlBlock *) shmat(*pcb_id, NULL, SHM_RND);
    on_exit(cleanupProcessControlBlock, pcb_id);

    //initialize RAM to all empty
    char RAM[pcb->RAM_size];
    int i;
    for(i=0; i<pcb->RAM_size; i++)
        RAM[i] = '.';

    while(true) //infinite loop
    {
        //try to add any jobs not in RAM
        placeJobsIntoRAM(pcb, RAM);
        //display RAM and jobs
        display(pcb, RAM, row_count, col_count);
        //sleep 1 second
        sleep(1);
        //decrement durations for processes in RAM
        decrementDurations(pcb);
        //remove any dead processes
        removeJobs(pcb, RAM);
    }
}

int main(int argc, char const *argv[])
{   
    //gracefully exit when killed
    //catch multiple types of kill signals
    signal(SIGTERM, exit);
    signal(SIGINT, exit);
    signal(SIGQUIT, exit);
    signal(SIGTSTP, exit);

    int row_count, col_count, buffer_size;
    parseArgs(argc, argv, &row_count, &col_count, &buffer_size);
    int RAM_size = row_count*col_count;

    writeToDeathRow(getpid()); //tell shutdown this process should be killed

    static int pcb_id;
    pcb_id = setupProcessControlBlock(RAM_size);

    spawnBufferConsumer(buffer_size, RAM_size, pcb_id);
    manageMemory(&pcb_id, row_count, col_count);
    
    return 0;
}
