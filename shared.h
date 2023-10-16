#ifndef SHARED_H
#define SHARED_H

// Define a custom signal to notify child process to start execution
#define SIG_START_EXECUTION (SIGRTMIN + 1)
#define SIG_STOP_EXECUTION (SIGRTMIN + 2)
#define SIG_NEW_PROGRAM (SIGRTMIN + 3)
#define MAX_QUEUE_SIZE 100

// Data structure to represent a job or process
struct Process
{
    int pid;            // Process ID
    char command[1024]; // Command to execute
};

// Data structure to represent a process queue
struct ProcessQueue
{
    struct Process processes[MAX_QUEUE_SIZE];
    int front; // Index of the front element
    int rear;  // Index of the rear element
};
extern struct ProcessQueue *sharedQueue;

int initSharedResources(key_t *shmkey, key_t *semkey, int *shmid, int *semid, struct ProcessQueue **processQueue);
void cleanupSharedResources(int shmid, int semid, struct ProcessQueue *processQueue);

// Initialize the queue
void initializeQueue(struct ProcessQueue *queue);

// Check if the queue is empty
int isQueueEmpty(struct ProcessQueue *queue);

// Check if the queue is full
int isQueueFull(struct ProcessQueue *queue);

// Enqueue a process
void enqueue(struct ProcessQueue *queue, struct Process *process);

// Dequeue a process
struct Process dequeue(struct ProcessQueue *queue);

// Function to send a signal to a process by its PID
int sendSignalToProcess(int pid, int signal);

// Initialize signal handling for custom signals
void initializeSignalHandling();

#endif
