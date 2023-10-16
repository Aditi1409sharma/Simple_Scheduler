#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <time.h>
#include "shared.h"

// The size of the shared memory segment
#define SHARED_MEMORY_SIZE 1024
void daemonize()
{
    pid_t pid, sid;
    pid = fork();
    if (pid < 0)
    {
        exit(1); // Fork error
    }
    if (pid > 0)
    {
        exit(0); // Parent process exits
    }
    umask(0);       // Change file mode mask
    sid = setsid(); // Create a new session
    if (sid < 0)
    {
        exit(1); // SID creation error
    }
    // Change the working directory to a safe location
    if ((chdir("/")) < 0)
    {
        exit(1);
    }
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}

// Function to manage the round-robin scheduling
void scheduleProcesses(int NCPU, int TSLICE, struct Process *processes, int numProcesses, int *shm_id, struct ProcessQueue *sharedQueue)
{
    int front = sharedQueue->front;

    while (numProcesses > 0)
    {
        int runningCount = 0;

        for (int i = 0; i < NCPU; i++)
        {
            if (!isQueueEmpty(sharedQueue))
            {
                struct Process currentProcess = dequeue(sharedQueue);
                int processIndex = currentProcess.pid;

                // Calculate waiting time for the process
                time_t currentTime;
                time(&currentTime);
                currentProcess.waitingTime += difftime(currentTime, currentProcess.arrivalTime);

                // Update the arrival time when the process leaves the queue
                time(&currentProcess.arrivalTime);

                // Notify the child process to start execution
                sendSignalToProcess(currentProcess.pid, SIG_START_EXECUTION);
                printf("Scheduler: Process %d started execution (Waiting Time: %.2lf seconds).\n", currentProcess.pid, currentProcess.waitingTime);
                runningCount++;
            }
        }

        sleep(TSLICE);

        for (int i = 0; i < NCPU; i++)
        {
            if (!isQueueEmpty(sharedQueue))
            {
                struct Process currentProcess = dequeue(sharedQueue);

                // Update the arrival time when the process leaves the queue
                time(&currentProcess.arrivalTime);

                // Calculate the execution end time and update it
                double executionDuration = difftime(currentProcess.executionEndTime, currentProcess.arrivalTime);
                currentProcess.waitingTime += executionDuration;
                currentProcess.executionEndTime = 0;

                // Notify the child process to stop execution
                sendSignalToProcess(currentProcess.pid, SIG_STOP_EXECUTION);
                printf("Scheduler: Process %d stopped execution (Total Waiting Time: %.2lf seconds).\n", currentProcess.pid, currentProcess.waitingTime);
            }
        }
    }

    if (shmdt(processes) == -1)
    {
        perror("shmdt");
    }

    if (shmctl(*shm_id, IPC_RMID, NULL) == -1)
    {
        perror("shmctl");
    }
}

int main(int argc, char *argv[])
{
    daemonize();

    key_t shmkey;
    int shmid;

    key_t history_shmkey = ftok("history_shared_memory_key", 1024);
    int history_shmid = shmget(history_shmkey, sizeof(struct CommandHistory), 0666);

    if (history_shmid == -1)
    {
        perror("shmget for history");
        exit(1);
    }

    struct CommandHistory *history = (struct CommandHistory *)shmat(history_shmid, NULL, 0);

    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <NCPU> <TSLICE>\n", argv[0]);
        exit(1);
    }

    int NCPU = atoi(argv[1]);
    int TSLICE = atoi(argv[2]);

    key_t key = ftok("shared_memory_key", 65);
    int shm_id = shmget(key, SHARED_MEMORY_SIZE, IPC_CREAT | 0666);

    if (shm_id == -1)
    {
        perror("shmget");
        exit(1);
    }

    struct Process *processes = (struct Process *)shmat(shm_id, NULL, 0);

    if (processes == (void *)-1)
    {
        perror("shmat");
        exit(1);
    }

    int numProcesses = 0;

    while (1)
    {
        if (processes[numProcesses].pid != -1)
        {
            numProcesses++;
        }
        else
        {
            break;
        }
    }

    initializeSignalHandling();

    // Access the shared queue from shared.c
    key_t semkey;
    int semid;
    struct ProcessQueue *sharedQueue;

    // Initialize shared resources and obtain the shared queue
    if (initSharedResources(&shmkey, &semkey, &shmid, &semid, &sharedQueue) != 0)
    {
        exit(1);
    }

    // Call the scheduling function
    scheduleProcesses(NCPU, TSLICE, processes, numProcesses, &shm_id, sharedQueue);

    cleanupSharedResources(shmid, semid, sharedQueue);

    return 0;
}