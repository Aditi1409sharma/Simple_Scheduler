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
#include "shared.h"

#define MAX_COMMAND_LENGTH 1024
#define MAX_HISTORY_SIZE 100

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

// Structure to store shared data (NCPU and TSLICE)
struct SharedData
{
    int NCPU;
    int TSLICE;
};

struct CommandHistory history;

void stopProcess(int signo)
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

void timerExpired(int signo)
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

void startExecutionHandler(int signo)
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
        history->pids[history->count] = pid;
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

    // Create a shared memory segment
    key_t key = ftok("shared_memory_key", 1);
    int shmid = shmget(key, sizeof(struct SharedData), 0666 | IPC_CREAT);

    if (shmid == -1)
    {
        perror("shmget failed");
        exit(1);
    }

    // Attach the shared memory segment
    struct SharedData *sharedData = (struct SharedData *)shmat(shmid, NULL, 0);
    if ((int)sharedData == -1)
    {
        perror("shmat failed");
        exit(1);
    }

    // Store NCPU and TSLICE values in shared memory
    sharedData->NCPU = NCPU;
    sharedData->TSLICE = TSLICE;

    // Initialize the signal handlers
    signal(SIG_START_EXECUTION, startExecutionHandler);
    signal(SIG_NEW_PROGRAM, newProgramHandler);

    // The rest of your code remains the same

    while (1)
    {
        // The main loop of your program
    }

    // Detach and remove the shared memory segment when done
    shmdt(sharedData);
    shmctl(shmid, IPC_RMID, NULL);

    return 0;
}
