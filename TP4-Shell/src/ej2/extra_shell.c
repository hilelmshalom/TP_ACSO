#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

#define MAX_COMMANDS 200 // Maximum number of commands in a pipeline
#define MAX_ARGS 64      // Maximum number of arguments for a single command

int main() {
    char command[256];         // Buffer for the raw input line
    char *commands[MAX_COMMANDS]; // Array to store pointers to command segments (separated by '|')
    
    while (1) {
        int command_count = 0; // Reset count for each new input line

        printf("Shell> ");
        fflush(stdout); // Ensure prompt is displayed before fgets

        // Reads a line of input from the user
        if (fgets(command, sizeof(command), stdin) == NULL) {
            // Handle EOF (Ctrl+D)
            printf("\nExiting Shell.\n");
            break; 
        }

        // Removes the newline character (\n) from the end of the string
        command[strcspn(command, "\n")] = '\0';

        // Handle empty command
        if (command[0] == '\0') {
            continue;
        }

        // Tokenizes the command string using the pipe character (|) as a delimiter.
        char *token = strtok(command, "|");
        while (token != NULL && command_count < MAX_COMMANDS) {
            commands[command_count++] = token;
            token = strtok(NULL, "|");
        }

        if (command_count == 0) { // Should not happen if command[0] != '\0'
            continue;
        }
        
        // Special command: "exit"
        // We need to trim whitespace from the first segment to check for "exit"
        char *first_command_trimmed = commands[0];
        while(*first_command_trimmed == ' ' || *first_command_trimmed == '\t') first_command_trimmed++; // Trim leading whitespace
        char *end_ptr = first_command_trimmed + strlen(first_command_trimmed) - 1;
        while(end_ptr > first_command_trimmed && (*end_ptr == ' ' || *end_ptr == '\t')) {
            *end_ptr = '\0';
            end_ptr--;
        }
        if (command_count == 1 && strcmp(first_command_trimmed, "exit") == 0) {
            printf("Exiting Shell.\n");
            break;
        }


        int num_pipes = command_count - 1;
        int pipefds[2 * num_pipes]; // Array to hold all pipe file descriptors
                                    // Each pipe needs 2 fds: pipefds[i*2] for read, pipefds[i*2+1] for write

        pid_t pids[command_count];  // Array to store PIDs of child processes

        // Create all necessary pipes before forking children
        for (int i = 0; i < num_pipes; i++) {
            if (pipe(pipefds + i * 2) < 0) {
                perror("pipe creation failed");
                // exit(EXIT_FAILURE); // In a real shell, might try to cleanup/continue
                goto cleanup_pipes_and_continue; // Jump to cleanup before next prompt
            }
        }

        // --- Main Execution Loop for Piped Commands ---
        for (int i = 0; i < command_count; i++) {
            // Parse the current command segment (e.g., "ls -l") into command and arguments
            char *current_segment_str = strdup(commands[i]); // Duplicate because strtok modifies
            if (current_segment_str == NULL) {
                perror("strdup failed");
                // This is a critical error, consider exiting or more robust recovery
                // For simplicity, we'll try to clean up and continue to next prompt
                for(int k=0; k<i; k++) waitpid(pids[k], NULL, 0); // Wait for already started children
                goto cleanup_pipes_and_continue;
            }

            char *args[MAX_ARGS];    // Array for execvp arguments
            int arg_count = 0;
            char *arg_token = strtok(current_segment_str, " \t"); // Tokenize by space or tab

            while (arg_token != NULL && arg_count < MAX_ARGS - 1) {
                args[arg_count++] = arg_token;
                arg_token = strtok(NULL, " \t");
            }
            args[arg_count] = NULL; // execvp expects a NULL terminated array

            if (arg_count == 0) { // Empty command segment (e.g., "ls | | wc")
                fprintf(stderr, "Shell: syntax error near unexpected token `|' (empty command segment)\n");
                free(current_segment_str);
                // Need to ensure other children are handled/waited for and pipes closed
                // This complexifies cleanup. For now, break and let outer cleanup handle.
                // Mark that an error occurred to skip waiting for this specific (non-existent) child.
                pids[i] = -1; // Mark as invalid PID
                continue; // Skip forking this empty command
            }

            pids[i] = fork();
            if (pids[i] < 0) {
                perror("fork failed");
                free(current_segment_str);
                // Attempt to cleanup previously created children and pipes
                for(int k=0; k<i; k++) if(pids[k] > 0) waitpid(pids[k], NULL, 0);
                goto cleanup_pipes_and_continue;
            }

            if (pids[i] == 0) { // --- Child Process ---
                // Input Redirection: If not the first command, redirect stdin from the previous pipe's read end
                if (i > 0) { // i.e., this is not the first command in the pipeline
                    if (dup2(pipefds[(i - 1) * 2], STDIN_FILENO) < 0) {
                        perror("dup2 stdin failed");
                        exit(EXIT_FAILURE);
                    }
                }

                // Output Redirection: If not the last command, redirect stdout to the current pipe's write end
                if (i < num_pipes) { // i.e., this is not the last command in the pipeline
                    if (dup2(pipefds[i * 2 + 1], STDOUT_FILENO) < 0) {
                        perror("dup2 stdout failed");
                        exit(EXIT_FAILURE);
                    }
                }

                // Close ALL pipe file descriptors in the child.
                // The child has its stdin/stdout redirected (if needed) via dup2.
                // It does not need direct access to any of the original pipe fds.
                for (int j = 0; j < 2 * num_pipes; j++) {
                    close(pipefds[j]);
                }

                // Execute the command
                execvp(args[0], args);
                // If execvp returns, an error occurred
                perror("execvp failed");
                fprintf(stderr, "Failed to execute: %s\n", args[0]);
                exit(EXIT_FAILURE); // Exit child process on execvp error
            }
            // --- Parent Process (continues in the loop to fork next child) ---
            free(current_segment_str); // Free the duplicated string for arguments
        }

    cleanup_pipes_and_continue:; // Label for goto, ensures pipes are closed

        // --- Parent Process (after forking all children) ---
        // Close all pipe file descriptors in the parent.
        // This is crucial. If the parent doesn't close the write ends,
        // children reading from pipes might not get EOF and could hang.
        for (int i = 0; i < 2 * num_pipes; i++) {
            close(pipefds[i]);
        }

        // Wait for all child processes to finish
        for (int i = 0; i < command_count; i++) {
            if (pids[i] > 0) { // Only wait for valid PIDs
                int status;
                waitpid(pids[i], &status, 0);
                // Optionally, check child exit status with WIFEXITED, WEXITSTATUS, etc.
            }
        }
    } // End of while(1) loop

    return 0;
}
