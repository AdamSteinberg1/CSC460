// Adam Steinberg
// Christina's Cafe
// CSC 460
// 04/04/2022

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define PHILOSOPHER_COUNT 5
#define CLOCK shmem[PHILOSOPHER_COUNT]
#define PHILOSOPHER_STATE(i) shmem[i]
#define MUTEX PHILOSOPHER_COUNT
#define LEFT (i+PHILOSOPHER_COUNT-1)%PHILOSOPHER_COUNT
#define RIGHT (i+1)%PHILOSOPHER_COUNT

enum state {thinking, hungry, eating, dead};

int semid;
int shmid;
int* shmem;
pid_t philosopher_pids[PHILOSOPHER_COUNT];

void think()
{
    int duration = 5 + rand()%11; //random int in 5-15
    sleep(duration);
}

void eat()
{
    int duration = 1 + rand()%3; //random int in 1-3
    sleep(duration);
}

p(int s,int semid)
{
    struct sembuf sops;

    sops.sem_num = s;
    sops.sem_op = -1;
    sops.sem_flg = 0;
    if((semop(semid, &sops, 1)) == -1)
        printf("'P' error\n");
}

v(int s, int semid)
{
    struct sembuf sops;

    sops.sem_num = s;
    sops.sem_op = 1;
    sops.sem_flg = 0;
    if((semop(semid, &sops, 1)) == -1) 
        printf("'V' error\n");
}

void test(int i)
{
    if( PHILOSOPHER_STATE(i) == hungry && 
        PHILOSOPHER_STATE(LEFT) != eating && 
        PHILOSOPHER_STATE(RIGHT) != eating)
    {
        PHILOSOPHER_STATE(i) = eating;
        v(i, semid);
    }
}

void take_chopsticks(int i)
{
    p(MUTEX, semid);
    PHILOSOPHER_STATE(i) = hungry;
    test(i);
    v(MUTEX, semid);
    p(i, semid);
}

void put_chopsticks(int i)
{
    p(MUTEX, semid);
    PHILOSOPHER_STATE(i) = thinking;
    test(LEFT);
    test(RIGHT);
    v(MUTEX, semid);
}

//starts the ith philosopher 
void philosopher(int i)
{
    srand(getpid());
    while(CLOCK < 100)
    {
        think();
        if(CLOCK >= 100) break;
        take_chopsticks(i);
        eat();
        put_chopsticks(i);
    }
    PHILOSOPHER_STATE(i) = dead;
}

//gets shared resources
void setup()
{
    //get semaphores, 1 for each philosopher and 1 for mutex
    semid = semget(IPC_PRIVATE, PHILOSOPHER_COUNT+1, 0700);
    if (semid == -1)
    {
        printf("Error: semget failed.\n");
        exit(1);
    }
    
    //initialize MUTEX to 1
    semctl(semid, MUTEX, SETVAL, 1);  

    //initialize other semaphores to 0
    int i;
    for(i=0;i<PHILOSOPHER_COUNT;i++)
        semctl(semid, i, SETVAL, 0);   

    //get shared memory
    //shared memory has 1 slot for each philosopher state and an extra slot for current time
    shmid = shmget(IPC_PRIVATE, (PHILOSOPHER_COUNT+1)*sizeof(int), 0700);
    if (shmid == -1)
    {
        printf("Error: shmget failed.\n");
        exit(1);
    }
    //attach to shared memory
    shmem = (int *) shmat(shmid, NULL, SHM_RND);
    //clock starts at 0 seconds
    CLOCK = 0; 
}

//removes shared resources and kills child processes
void cleanup()
{   
    //remove semaphores
    if (semctl(semid, 0, IPC_RMID, 0) == -1)
        printf("Error removing semaphores.\n");

    //remove shared memory
    if (shmctl(shmid, IPC_RMID, NULL) == -1)
        printf("Error removing shared memory.\n");

    //kill child processes if there are any
    int i;
    for(i=0; i<PHILOSOPHER_COUNT; i++)
    {
        int childPid = philosopher_pids[i];
        kill(childPid, SIGTERM);
    }
}

//return true if all philosophers have died
bool all_dead()
{
    int i;
    for(i=0; i<PHILOSOPHER_COUNT; i++)
    {
        if(PHILOSOPHER_STATE(i) != dead)
        {
            return false;
        }
    }
    return true;
}


//prints out the clock and states of all philosophers
void printStates() 
{
    printf("%d.", CLOCK);
    int i = 0;
    for(i=0; i<PHILOSOPHER_COUNT; i++)
    {
        printf("\t");
        switch(PHILOSOPHER_STATE(i))
        {
            case thinking:
                printf("thinking");
                break;
            case hungry:
                printf("hungry\t");
                break;
            case eating:
                printf("eating\t");
                break;
            case dead:
                printf("dead\t");
                break;
        }
    }
    printf("\n");
}

int main()
{
    //gracefully exit when killed
    //catch multiple types of kill signals
    signal(SIGTERM, exit);
    signal(SIGINT, exit);
    signal(SIGQUIT, exit);
    signal(SIGTSTP, exit);

    setup();

    int i;
    for(i=0; i<PHILOSOPHER_COUNT; i++)
    {
        if((philosopher_pids[i]=fork())==0)
        {
            philosopher(i);
            exit(0);
        }
    }

    atexit(cleanup); //the parent should cleanup shared resources at exit
    
    while(!all_dead()) //while there are philosophers alive
    {
        //increment the time once every second
        sleep(1);
        CLOCK++;
        printStates();
    }
}
