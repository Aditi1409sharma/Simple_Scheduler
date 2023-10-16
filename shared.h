#ifndef SHARED_H
#define SHARED_H

// Define a custom signal to notify child process to start execution
#define SIG_START_EXECUTION (SIGRTMIN + 1)
#define SIG_STOP_EXECUTION (SIGRTMIN + 2)
#define SIG_NEW_PROGRAM (SIGRTMIN + 3)
#define MAX_QUEUE_SIZE 100
#define MAX_HISTORY_SIZE 100
#define MAX_COMMAND_LENGTH 1024
struct CommandHistory
{
    char commands[MAX_HISTORY_SIZE][MAX_COMMAND_LENGTH];
    int count;
    struct timeval start_time[MAX_HISTORY_SIZE];
    struct timeval end_time[MAX_HISTORY_SIZE]; // Add end_time field to store end time
    double duration[MAX_HISTORY_SIZE];
    int pids[MAX_HISTORY_SIZE];
    int priorities[MAX_HISTORY_SIZE];
};

// Data structure to represent a job or process
struct Process
{
    pid_t pid;                        // Process ID
    char command[MAX_COMMAND_LENGTH]; // Command to execute
    time_t arrivalTime;               // Arrival time
    time_t executionEndTime;          // Execution end time
    double waitingTime;               // Waiting time
};

// Data structure to represent a process queue
struct ProcessQueue
{
    struct Process processes[MAX_QUEUE_SIZE];
    int front; // Index of the front element
    int rear;  // Index of the rear element
};
extern struct ProcessQueue *sharedQueue;
extern struct CommandHistory *history;

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
