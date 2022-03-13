// Adam Steinberg
// Crypto Club Wallet
// CSC 460
// 03/18/2022

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#define BALANCE shmem[0]
#define MUTEX 0

#define FILENAME "cryptodata"


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

//reads the shm_id and sem_id from file and returns them through the arguments
void getIDs(int* shm_id, int* sem_id)
{
    FILE *fp;
    if((fp = fopen(FILENAME,"r")) == NULL)
    {
        printf("Error: could not open %s to read.\n", FILENAME);
        exit(1);
    }
    fscanf(fp, "%d\n%d", shm_id, sem_id);
    fclose(fp);
}

void setup()
{   
    //make sure system is not already started
    if(access(FILENAME, F_OK) == 0)
    {
        printf("Error: Crypto wallet has already been started. Run with the argument \"cleanup\" to stop the system.\n");
        exit(1);
    }

    //get shared memory
    int shm_id = shmget(IPC_PRIVATE, sizeof(int), 0777);
    if (shm_id == -1)
    {
        printf("Error: shmget failed.\n");
        exit(1);
    }
    int* shmem = (int *) shmat(shm_id, NULL, SHM_RND);

    //initialize wallet to 1000 coins
    BALANCE = 1000; 

    //get one semaphore
    int sem_id = semget (IPC_PRIVATE, 1, 0777);
    if (sem_id == -1)
    {
        printf("%s","Error semget failed.\n");
        exit(1);
    }

    //initialize semaphore to 1
    semctl(sem_id, MUTEX, SETVAL, 1);

    //put IDs in file for subsequent runs
    FILE *fp;
    if((fp = fopen(FILENAME,"w")) == NULL)
    {
        printf("Error: could not open %s to write.\n", FILENAME);
        exit(1);
    }
    fprintf(fp, "%d\n%d", shm_id, sem_id);
    fclose(fp);
}

void printCoinCount()
{
    int shm_id, sem_id;
    getIDs(&shm_id, &sem_id);
    int* shmem = (int *) shmat(shm_id, NULL, SHM_RND);
    printf("Balance = %d\n", BALANCE);
}

void cleanup()
{
    int shm_id, sem_id;
    getIDs(&shm_id, &sem_id);

    //get and print coin count
    int* shmem = (int *) shmat(shm_id, NULL, SHM_RND);
    printf("Balance = %d\n", BALANCE);

    printf("Shutting down crypto wallet.\n");

    //remove shared memory
    if (shmctl(shm_id, IPC_RMID, NULL) == -1)
        printf("Error removing shared memory.\n");

    //remove semaphore
    if (semctl(sem_id, 0, IPC_RMID, 0) == -1)
        printf("Error removing semaphore\n");

    //delete file
    if(remove(FILENAME) != 0)
        printf("Error removing file \"%s\"\n", FILENAME);
}


//simulates one depositer/withdrawer pair
void simulatePair(const int n, const int amount, int* shmem, int sem_id)
{   
    if(fork())
    {
        //depositer
        int i;
        for(i=0; i<n; i++)
        {
            p(MUTEX, sem_id);

            //Critical Section
            int oldCoinCount = BALANCE;
            BALANCE += amount;
            printf("%d + %d = %d\n", oldCoinCount, amount, BALANCE);

            v(MUTEX, sem_id);
        }
    }
    else
    {
        //withdrawer
        int i;
        for(i=0; i<n; i++)
        {
            p(MUTEX, sem_id);

            //Critical Section
            int oldCoinCount = BALANCE;
            BALANCE -= amount;
            printf("\t\t\t\t%d - %d = %d\n", oldCoinCount, amount, BALANCE);

            v(MUTEX, sem_id);
        }
    }
}

//performs the simulation with n operations per process
void simulate(const int n)
{
    int shm_id, sem_id;
    getIDs(&shm_id, &sem_id);
    int* shmem = (int *) shmat(shm_id, NULL, SHM_RND);

    //create 16 processes
    int val;
    for(val=0; val<16; val++)
    {
        if(fork()==0)
        {
            const int amount = 1 << val; // 2^val
            simulatePair(n, amount, shmem, sem_id);
            return;
        }
    }
}


main(int argc, char *argv[])
{
    if(argc==1)
    {
        setup();
        exit(0);
    }

    //check if data file does not exist
    if(access(FILENAME, F_OK) != 0)
    {
        printf("Error: System has not been started. Run the program with no arguments first.\n");
        exit(1);
    }

    if(strcmp(argv[1], "coins") == 0)
    {
        printCoinCount();
        exit(0);
    }
    if(strcmp(argv[1], "cleanup") == 0)
    {
        cleanup();
        exit(0);
    }
    if(!isNum(argv[1]))
    {
        printf("Error: %s is an invalid argument\n", argv[1]);
        exit(1);
    }
    const int n = atoi(argv[1]);
    if(n < 1 || n > 100)
    {
        printf("Error: %d is outside the acceptable range [1,100]\n", n);
        exit(1);
    }

    simulate(n);
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