#include <sys/ipc.h>
#include <sys/sem.h>
#include<sys/wait.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#define NUM_PHILOSOPHERS 5

int sem_id=-1;

//wastes a random amount of CPU cycles
void wasteTime()
{
    const int loopSize = rand() % 1000;
    int i, j;
    for(i=0;i<loopSize;i++)
        for(j=0; j<loopSize; j++)
            1+1;
}

void think(int i)
{
    int j;
    for(j=0; j<i; j++)
        printf("\t");
    printf("%d THINKING\n", i);
}

void getHungry(int i)
{
    int j;
    for(j=0; j<i; j++)
        printf("\t");
    printf("%d HUNGRY\n", i);
    
    //pick up left chopstick
    p(i, sem_id);

    wasteTime();

    //pick up right chopstick
    p((i+1)%NUM_PHILOSOPHERS, sem_id);

}

void eat(int i)
{
    int j;
    for(j=0; j<i; j++)
        printf("\t");
    printf("%d EATING\n", i);
    
    wasteTime();

    //set down left chopstick
    v(i, sem_id);
    //set down right chopstick
    v((i+1)%NUM_PHILOSOPHERS, sem_id);
}

//starts the ith philosopher who goes on forever
void philosopher(int i)
{
    srand(getpid());
    while(1)
    {
        think(i);
        getHungry(i);
        eat(i);
    }
}

void cleanupSemaphores()
{
    semctl(sem_id, 0, IPC_RMID, 0);
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

int main()
{
    //gracefully exit when killed
    //catch multiple types of kill signals
    signal(SIGTERM, exit);
    signal(SIGINT, exit);
    signal(SIGQUIT, exit);
    signal(SIGTSTP, exit);

    //get semaphores
    sem_id = semget(IPC_PRIVATE, NUM_PHILOSOPHERS, 0700);
    if (sem_id == -1)
    {
        printf("Error: semget failed.\n");
        exit(1);
    }

    //initialize the semaphores to 1
    int i;
    for(i=0;i<NUM_PHILOSOPHERS;i++)
        semctl(sem_id, i, SETVAL, 1);   

    for(i=0; i<NUM_PHILOSOPHERS; i++)
    {
        if(fork()==0)
        {
            philosopher(i);
            exit(0);
        }
    }

    //when the parent exits, cleanup semaphores
    atexit(cleanupSemaphores);

    //wait for all the children to finish, (likely will wait forever and must be killed)
    while (wait(NULL) > 0);
}
