#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "shared.h" // Include the shared header but not the shared.c file

#define MAX_COMMAND_LENGTH 1024
#define MAX_HISTORY_SIZE 100
#define MAX_QUEUE_SIZE 100

struct SharedData
{
    int NCPU;
    int TSLICE;
};

// Structure to store command history
struct CommandHistory
{
    char commands[MAX_HISTORY_SIZE][MAX_COMMAND_LENGTH];
    int count;
    time_t start_time[MAX_HISTORY_SIZE];
    double duration[MAX_HISTORY_SIZE];
    int pids[MAX_HISTORY_SIZE];
    int priorities[MAX_HISTORY_SIZE];
};

struct CommandHistory history;

void stopProcess(int NCPU, int running_pids[], int signo)
{
    // Stop the currently running process
    for (int i = 0; i < NCPU; i++)
    {
        if (running_pids[i] > 0)
        {
            kill(running_pids[i], SIGSTOP);
        }
    }
}

void timerExpired(int NCPU, int running_pids[], int signo)
{
    // Implement what happens when a timer expires
    // Resume the stopped processes
    for (int i = 0; i < NCPU; i++)
    {
        if (running_pids[i] > 0)
        {
            kill(running_pids[i], SIGCONT);
        }
    }
    // Re-arm the timer
    alarm(1);
}

void startExecutionHandler(pid_t pid, int signo)
{
    // This signal handler will be called when simpleScheduler signals to start execution
    // You should start the program execution here
    // Add your code to start the execution of the program
}

void newProgramHandler(int signo)
{
    // This signal handler will be called when a new program is ready to run
    // You can use this signal to notify simpleScheduler when a new program is submitted
    // Add your code to handle the arrival of a new program
}

void executeCommand(char *command, struct CommandHistory *history)
{
    pid_t pid;
    int status;
    int command_index = history->count;

    time(&history->start_time[command_index]);

    pid = fork();

    if (pid < 0)
    {
        perror("Fork failed");
        exit(1);
    }
    else if (pid == 0)
    { // Child process
        // Enqueue the process for scheduling
        struct Process newProcess;
        history->pids[history->count] = pid;
        strncpy(newProcess.command, command, MAX_COMMAND_LENGTH);
        enqueue(ProcessQueue, &newProcess);    
        if (strchr(command, '/'))
        {
            // Execute the command using the system function
            int system_status = system(command);

            if (system_status == -1)
            {
                perror("Command execution failed");
                exit(1);
            }
        }
        else
        {
            char *args[MAX_COMMAND_LENGTH];
            char *token;
            int arg_count = 0;

            // Tokenize the command
            token = strtok(command, " ");
            while (token != NULL)
            {
                args[arg_count++] = token;
                token = strtok(NULL, " ");
            }
            args[arg_count] = NULL;

            // Execute the command
            execvp(args[0], args);

            // If execvp fails, print an error message
            perror("Command execution failed");
            exit(1);
        }
    }
    else
    { // Parent process
        waitpid(pid, &status, 0);
        time_t end_time;
        time(&end_time);
        history->duration[command_index] = difftime(end_time, history->start_time[command_index]);
    }
}

void displayHistory(struct CommandHistory history)
{
    printf("Command History:\n");
    for (int i = 0; i < history.count; i++)
    {
        printf("%d: %s\n", i + 1, history.commands[i]);
        printf("    PID : %d\n", history.pids[i]);
        printf("    Priority: %d\n", history.priorities[i]);
        printf("    Start Time : %s", ctime(&history.start_time[i]));
        printf("    Duration : %.3lf seconds\n", history.duration[i]);
    }
}

int main()
{
    int NCPU;
    int TSLICE;

    // Get the number of CPUs (NCPU) from the user
    printf("Enter the number of CPUs (NCPU): ");
    scanf("%d", &NCPU);

    // Get the time quantum (TSLICE) from the user
    printf("Enter the time quantum (TSLICE in seconds): ");
    scanf("%d", &TSLICE);

    key_t shmkey;
    key_t semkey;
    int shmid;
    int semid;
    struct ProcessQueue *processQueue;

    shmkey = ftok("shared_memory_key", 65);
    shmid = shmget(shmkey, sizeof(struct SharedData), IPC_CREAT | 0666);

    if (shmid == -1)
    {
        perror("shmget");
        exit(1);
    }

    // Attach the shared data to shared memory
    struct SharedData *sharedData = (struct SharedData *)shmat(shmid, NULL, 0);

    if (sharedData == (void *)-1)
    {
        perror("shmat failed");
        exit(1);
    }

    // Now you can set the initial values
    sharedData->NCPU = NCPU;
    sharedData->TSLICE = TSLICE;

    // Initialize shared resources here, similar to what you had in your original code
    // ...

    // Store NCPU and TSLICE values in shared memory

    // Initialize the signal handlers
    signal(SIG_START_EXECUTION, startExecutionHandler);
    signal(SIG_NEW_PROGRAM, newProgramHandler);

    // The rest of your code remains the same
    // ...

    while (1)
    {
        printf("SimpleShell> ");
        char input[MAX_COMMAND_LENGTH];
        fgets(input, sizeof(input), stdin);

        // Remove newline character from the input
        input[strcspn(input, "\n")] = '\0';

        if (strncmp(input, "submit ", 7) == 0)
        {
            // Extract the submitted command without "submit"
            char *command = input + 7;

            // Execute the command
            executeCommand(command, &history);

            // Store the command in history
            if (history.count < MAX_HISTORY_SIZE)
            {
                strcpy(history.commands[history.count], command);
                history.count++;
            }
            else
            {
                perror("History is full, can't add more commands");
            }
        }
        else if (strcmp(input, "history") == 0)
        {
            displayHistory(history);
        }
        else if (strcmp(input, "exit") == 0)
        {
            displayHistory(history);
            exit(0);
        }
    }

    // Detach and remove the shared memory segment when done
    shmdt(sharedData);
    shmctl(shmid, IPC_RMID, NULL);
    cleanupSharedResources(shmid, semid, processQueue);
    return 0;
}
