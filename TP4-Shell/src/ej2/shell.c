#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

#define MAX_COMMANDS 200
#define MAX_ARGS 64

int main() {

    char command[256];
    char *commands[MAX_COMMANDS];
    int command_count = 0;

    while (1) 
    {
        printf("Shell> ");
        fflush(stdout);
        
        /*Reads a line of input from the user from the standard input (stdin) and stores it in the variable command */
        if(fgets(command, sizeof(command), stdin) == NULL) 
        {
            /* If fgets() returns NULL, it indicates an error or end-of-file (EOF). 
               In this case, the shell will exit gracefully. */
            printf("\nExiting Shell.\n");
            break; 
        }
        
        /* Removes the newline character (\n) from the end of the string stored in command, if present. 
           This is done by replacing the newline character with the null character ('\0').
           The strcspn() function returns the length of the initial segment of command that consists of 
           characters not in the string specified in the second argument ("\n" in this case). */
        command[strcspn(command, "\n")] = '\0';
        // Handle empty command
        if (command[0] == '\0') {
            continue;
        }

        /* Tokenizes the command string using the pipe character (|) as a delimiter using the strtok() function. 
           Each resulting token is stored in the commands[] array. 
           The strtok() function breaks the command string into tokens (substrings) separated by the pipe character |. 
           In each iteration of the while loop, strtok() returns the next token found in command. 
           The tokens are stored in the commands[] array, and command_count is incremented to keep track of the number of tokens found. */
        char *token = strtok(command, "|");
        while (token != NULL && command_count < MAX_COMMANDS) 
        {
            commands[command_count++] = token;
            token = strtok(NULL, "|");
        }
        if (command_count == 0) { // Should not happen if command[0] != '\0'
            continue;
        }

        /* You should start programming from here... */
        for (int i = 0; i < command_count; i++) 
        {
            printf("Command %d: %s\n", i, commands[i]);
        }    
    }
    return 0;
}
