#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include "shared.h"

#define SHM_SIZE 1024

union semun
{
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

// Function to initialize shared memory and semaphores
int initSharedResources(key_t *shmkey, key_t *semkey, int *shmid, int *semid, struct ProcessQueue **processQueue)
{
    // Create a key for the shared memory segment
    *shmkey = ftok("shmkeyfile", 'X');

    // Create a shared memory segment
    *shmid = shmget(*shmkey, SHM_SIZE, IPC_CREAT | 0666);
    if (*shmid == -1)
    {
        perror("shmget");
        return -1;
    }

    // Attach the shared memory segment
    *processQueue = (struct ProcessQueue *)shmat(*shmid, NULL, 0);
    if (*processQueue == (void *)-1)
    {
        perror("shmat");
        return -1;
    }

    // Create or get the semaphore set
    *semkey = ftok("semkeyfile", 'X');
    *semid = semget(*semkey, 1, IPC_CREAT | 0666);
    if (*semid == -1)
    {
        perror("semget");
        return -1;
    }

    // Initialize the semaphore to 1 (unlocked)
    union semun sem_union;
    sem_union.val = 1;
    if (semctl(*semid, 0, SETVAL, sem_union) == -1)
    {
        perror("semctl");
        return -1;
    }

    return 0; // Success
}

// Function to clean up shared resources
void cleanupSharedResources(int shmid, int semid, struct ProcessQueue *processQueue)
{
    // Detach from shared memory
    if (shmdt(processQueue) == -1)
    {
        perror("shmdt");
    }

    // Remove the shared memory and semaphores
    shmctl(shmid, IPC_RMID, NULL);
    semctl(semid, 0, IPC_RMID);
}

// ... Other functions from your original shared.c ...

// Function to send a signal to a process by its PID
int sendSignalToProcess(int pid, int signal)
{
    if (kill(pid, signal) == -1)
    {
        perror("kill");
        return -1;
    }
    return 0; // Success
}

// Initialize signal handling for custom signals
void initializeSignalHandling()
{
    // Add your signal handling code here
}

void initializeQueue(struct ProcessQueue *queue)
{
    queue->front = -1;
    queue->rear = -1;
}

// Check if the queue is empty
int isQueueEmpty(struct ProcessQueue *queue)
{
    return queue->front == -1;
}

// Check if the queue is full
int isQueueFull(struct ProcessQueue *queue)
{
    return (queue->front == 0 && queue->rear == MAX_QUEUE_SIZE - 1) || (queue->rear == (queue->front - 1) % (MAX_QUEUE_SIZE - 1));
}

// Enqueue a process
void enqueue(struct ProcessQueue *queue, struct Process *process)
{
    if (isQueueFull(queue))
    {
        printf("Queue is full. Cannot enqueue.\n");
        return;
    }

    if (queue->front == -1)
    {
        // If the queue is empty, initialize front and rear
        queue->front = queue->rear = 0;
    }
    else
    {
        // If the rear is at the end, wrap around
        queue->rear = (queue->rear + 1) % MAX_QUEUE_SIZE;
    }

    // Enqueue the process
    queue->processes[queue->rear] = *process;
}

// Dequeue a process
struct Process dequeue(struct ProcessQueue *queue)
{
    struct Process emptyProcess;
    emptyProcess.pid = -1;

    if (isQueueEmpty(queue))
    {
        printf("Queue is empty. Cannot dequeue.\n");
        return emptyProcess;
    }

    struct Process dequeuedProcess = queue->processes[queue->front];

    if (queue->front == queue->rear)
    {
        // If there is only one element in the queue, reset the queue
        queue->front = queue->rear = -1;
    }
    else
    {
        queue->front = (queue->front + 1) % MAX_QUEUE_SIZE;
    }

    return dequeuedProcess;
}