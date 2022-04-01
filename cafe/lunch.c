// Adam Steinberg
// Christina's Cafe
// CSC 460
// 04/04/2022

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
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

int sem_id;
int shmid;
int* shmem;

void think()
{
    int duration = 5 + rand()%11; //random int in 5-15
    sleep(duration);
}

void test(int i)
{
    if( PHILOSOPHER_STATE(i) == hungry && 
        PHILOSOPHER_STATE(LEFT) != eating && 
        PHILOSOPHER_STATE(RIGHT) != eating)
    {
        PHILOSOPHER_STATE(i) = eating;
        v(i, sem_id);
    }
}

void take_chopsticks(int i)
{
    p(MUTEX, sem_id);
    PHILOSOPHER_STATE(i) = hungry;
    test(i);
    v(MUTEX, sem_id);
    p(i, sem_id);
}

void put_chopsticks(int i)
{
    p(MUTEX, sem_id);
    PHILOSOPHER_STATE(i) = thinking;
    test(LEFT);
    test(RIGHT);
    v(MUTEX, sem_id);
}

void eat()
{
    int duration = 1 + rand()%3; //random int in 1-3
    sleep(duration);
}

//starts the ith philosopher 
void philosopher(int i)
{
    srand(getpid());
    while(CLOCK < 100)
    {
        think();
        take_chopsticks(i);
        eat();
        put_chopsticks(i);
    }
    PHILOSOPHER_STATE(i) = dead;
}

void setup()
{
    //get semaphores, 1 for each philosopher and 1 for mutex
    sem_id = semget(IPC_PRIVATE, PHILOSOPHER_COUNT+1, 0700);
    if (sem_id == -1)
    {
        printf("Error: semget failed.\n");
        exit(1);
    }
    
    //initialize the semaphores to 1
    int i;
    for(i=0;i<PHILOSOPHER_COUNT+1;i++)
        semctl(sem_id, i, SETVAL, 1);   

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

void cleanup()
{
    if (semctl(sem_id, 0, IPC_RMID, 0) == -1)
        printf("Error removing semaphores.\n");

    if (shmctl(shmid, IPC_RMID, NULL) == -1)
        printf("Error removing shared memory.\n");
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

p(int s,int sem_id)
{
    struct sembuf sops;

    sops.sem_num = s;
    sops.sem_op = -1;
    sops.sem_flg = 0;
    if((semop(sem_id, &sops, 1)) == -1)
        printf("'P' error\n");
}

v(int s, int sem_id)
{
    struct sembuf sops;

    sops.sem_num = s;
    sops.sem_op = 1;
    sops.sem_flg = 0;
    if((semop(sem_id, &sops, 1)) == -1) 
        printf("'V' error\n");
}

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
    setup();

    int i;
    for(i=0; i<PHILOSOPHER_COUNT; i++)
    {
        if(fork()==0)
        {
            philosopher(i);
            exit(0);
        }
    }
    
    while(!all_dead()) //while there are philosophers alive
    {
        //increment the time once every second
        sleep(1);
        CLOCK++;
        printStates();
    }

    cleanup();
}
