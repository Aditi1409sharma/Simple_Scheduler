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
};

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

void executePipeCommands(char *commands, struct CommandHistory *history)
{
    char **pipe_commands = NULL;
    char *token;
    int num_commands = 0;
    int command_index = history->count;
    time(&history->start_time[command_index]);

    token = strtok(commands, "|");
    while (token != NULL)
    {
        char *command = (char *)malloc(strlen(token) + 1);
        if (command == NULL)
        {
            perror("Memory allocation for command failed");
            exit(1);
        }
        strcpy(command, token);

        pipe_commands = (char **)realloc(pipe_commands, (num_commands + 1) * sizeof(char *));
        if (pipe_commands == NULL)
        {
            perror("Memory allocation for pipe_commands failed");
            exit(1);
        }
        pipe_commands[num_commands++] = command;
        token = strtok(NULL, "|");
    }
    pipe_commands[num_commands] = NULL;

    int pipe_fds[MAX_COMMAND_LENGTH - 1][2];
    pid_t child_pids[MAX_COMMAND_LENGTH];
    int status;

    for (int i = 0; i < num_commands; i++)
    {
        if (i < num_commands - 1)
        {
            if (pipe(pipe_fds[i]) == -1)
            {
                perror("Pipe creation failed");
                exit(1);
            }
        }
        child_pids[i] = fork();
        if (child_pids[i] < 0)
        {
            perror("Fork failed");
            exit(1);
        }
        else if (child_pids[i] == 0)
        {
            history->pids[history->count] = child_pids[i];
            if (i > 0)
            {
                if (dup2(pipe_fds[i - 1][0], 0) == -1) // Redirect stdin
                {
                    perror("Duplication of file descriptor failed");
                    exit(1);
                }
                close(pipe_fds[i - 1][0]);
            }

            if (i < num_commands - 1)
            {
                if (dup2(pipe_fds[i][1], 1) == -1) // Redirect stdout
                {
                    perror("Duplication of file descriptor failed");
                    exit(1);
                }
                close(pipe_fds[i][0]);
            }
            executeCommand(pipe_commands[i], history);

            exit(0);
        }
        else
        {
            if (i < num_commands - 1)
            {
                close(pipe_fds[i][1]);
            }
        }
    }
    for (int i = 0; i < (num_commands - 1); i++)
    {
        waitpid(child_pids[i], &status, 0);
        // free(pipe_commands[i]);
    }
    // free(pipe_commands);
    time_t end_time;
    time(&end_time);
    history->duration[command_index] = difftime(end_time, history->start_time[command_index]);
    history->count++;
}
// Function to display command history
void displayHistory(struct CommandHistory history)
{
    printf("Command History:\n");
    for (int i = 0; i < history.count; i++)
    {
        printf("%d: %s\n", i + 1, history.commands[i]);
        printf("    PID : %d\n", history.pids[i]);
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
        else if(strcmp(input, "exit")==0)
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
        
        else if (strchr(input, '|'))
        {
            executePipeCommands(input, &history);

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
