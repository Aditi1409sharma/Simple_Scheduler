#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>

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

// Global variables for process management
struct CommandHistory history;
int NCPU = 1; // Number of available CPUs
int current_priority = 1;
pid_t running_pids[4]; // Stores the PIDs of currently running processes

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

// Function to execute a command
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
    else
    { // Parent process
        waitpid(pid, &status, 0);
        time_t end_time;
        time(&end_time);
        history->duration[command_index] = difftime(end_time, history->start_time[command_index]);
    }
}

// Function to display command history
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
    char input[MAX_COMMAND_LENGTH];
    struct CommandHistory history;
    history.count = 0;

    while (1)
    {
        printf("SimpleShell> ");
        fgets(input, sizeof(input), stdin);

        // Remove newline character from the input
        input[strcspn(input, "\n")] = '\0';

        // Check for history command
        if (strcmp(input, "history") == 0)
        {
            displayHistory(history);
        }
        else if (strcmp(input, "exit") == 0)
        {
            displayHistory(history);
            exit(0);
        }
        else if (strncmp(input, "runscript ", 10) == 0)
        {
            char scriptFileName[MAX_COMMAND_LENGTH];
            strcpy(scriptFileName, input + 10);

            // Open the script file for reading
            FILE *scriptFile = fopen(scriptFileName, "r");

            if (scriptFile == NULL)
            {
                perror("Script file not found");
            }
            else
            {
                char scriptCommand[MAX_COMMAND_LENGTH];
                while (fgets(scriptCommand, sizeof(scriptCommand), scriptFile) != NULL)
                {
                    // Replace newline character with null terminator
                    scriptCommand[strcspn(scriptCommand, "\n")] = '\0';
                    executeCommand(scriptCommand, &history);
                }
                fclose(scriptFile);
            }
        }

        else
        {
            // Execute the command
            executeCommand(input, &history);

            // Store the command in history
            if (history.count < MAX_HISTORY_SIZE)
            {
                strcpy(history.commands[history.count], input);
                history.count++;
            }
            else
            {
                perror("History is full can't add more commands");
            }
        }
    }

    return 0;
}
