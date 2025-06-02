#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <ctype.h>      

#define MAX_COMMANDS 200
#define MAX_ARGS 256
#define MAX_PIPES (MAX_COMMANDS - 1)

// Función para parsear comandos con manejo de comillas
void parse_command(char *input, char **args) {
    int arg_count = 0;
    char *ptr = input;
    char *start = NULL;
    int in_quotes = 0;
    char quote_char = 0;
    
    while (*ptr) {
        if (!in_quotes && isspace(*ptr)) {
            if (start != NULL) {
                *ptr = '\0';
                args[arg_count++] = start;
                start = NULL;
            }
        }
        else if (!in_quotes && (*ptr == '"' || *ptr == '\'')) {
            quote_char = *ptr;
            in_quotes = 1;
            start = start ? start : ptr + 1;
        }
        else if (in_quotes && *ptr == quote_char) {
            in_quotes = 0;
            if (start) {
                *ptr = '\0';
                args[arg_count++] = start;
                start = NULL;
            }
        }
        else if (!in_quotes && *ptr == '\\' && *(ptr+1) != '\0') {
            *ptr = *(ptr+1);
            start = start ? start : ptr;
            ptr++;
        }
        else {
            if (start == NULL) {
                start = ptr;
            }
        }
        ptr++;
    }

    if (in_quotes) {
        fprintf(stderr, "Error: Comillas sin cerrar\n");
        args[0] = NULL;
        return;
    }

    if (start != NULL) {
        args[arg_count++] = start;
    }
    args[arg_count] = NULL;
}

// Función para validar la estructura del pipeline
int validate_pipeline(char **commands, int command_count) {
    // Verificar pipe al inicio
    if (command_count > 0 && strlen(commands[0]) == 0) {
        fprintf(stderr, "Error: Pipe al inicio\n");
        return 0;
    }
    
    // Verificar pipe al final
    if (command_count > 0 && strlen(commands[command_count-1]) == 0) {
        fprintf(stderr, "Error: Pipe al final\n");
        return 0;
    }
    
    // Verificar pipes consecutivos
    for (int i = 0; i < command_count; i++) {
        if (strlen(commands[i]) == 0) {
            fprintf(stderr, "Error: Pipe consecutivo\n");
            return 0;
        }
    }
    
    return 1;
}

int main() {
    char command[1024];
    char *commands[MAX_COMMANDS];
    int command_count = 0;

    while (1) {
        printf("Shell> ");
        fflush(stdout);

        if (!fgets(command, sizeof(command), stdin)) {
            printf("\n");
            break;
        }
        command[strcspn(command, "\n")] = '\0';

        if (strcmp(command, "exit") == 0) {
            break;
        }

        // Reset command processing
        command_count = 0;
        char *token = strtok(command, "|");
        while (token != NULL && command_count < MAX_COMMANDS) {
            // Trim espacios iniciales
            while (*token == ' ') token++;
            commands[command_count++] = token;
            token = strtok(NULL, "|");
        }

        // Saltar si no hay comandos válidos
        if (command_count == 0) {
            continue;
        }

        // Validar estructura del pipeline
        if (!validate_pipeline(commands, command_count)) {
            continue;
        }

        // Parsear cada comando en argumentos
        char *args_list[MAX_COMMANDS][MAX_ARGS];
        for (int i = 0; i < command_count; i++) {
            parse_command(commands[i], args_list[i]);
            
            // Verificar comando vacío después de parsear
            if (args_list[i][0] == NULL) {
                command_count = 0;
                break;
            }
        }
        
        if (command_count == 0) {
            continue;
        }

        // Comando único sin pipes
        if (command_count == 1) {
            pid_t pid = fork();
            if (pid == 0) {
                execvp(args_list[0][0], args_list[0]);
                perror("execvp");
                exit(EXIT_FAILURE);
            } else if (pid < 0) {
                perror("fork");
            } else {
                waitpid(pid, NULL, 0);
            }
        } 
        // Múltiples comandos con pipes
        else {
            int pipes[MAX_PIPES][2];
            pid_t pids[command_count];
            int status;
            
            // Crear todos los pipes necesarios
            for (int i = 0; i < command_count - 1; i++) {
                if (pipe(pipes[i]) == -1) {
                    perror("pipe");
                    return EXIT_FAILURE;
                }
            }
            
            // Crear procesos hijos
            for (int i = 0; i < command_count; i++) {
                if ((pids[i] = fork()) == -1) {
                    perror("fork");
                    // Cerrar pipes ya creados
                    for (int j = 0; j < command_count - 1; j++) {
                        close(pipes[j][0]);
                        close(pipes[j][1]);
                    }
                    // Esperar hijos ya creados
                    for (int j = 0; j < i; j++) {
                        waitpid(pids[j], &status, 0);
                    }
                    return EXIT_FAILURE;
                }
                
                if (pids[i] == 0) { // Proceso hijo
                    // Conectar entrada si no es el primer comando
                    if (i > 0) {
                        dup2(pipes[i-1][0], STDIN_FILENO);
                    }
                    
                    // Conectar salida si no es el último comando
                    if (i < command_count - 1) {
                        dup2(pipes[i][1], STDOUT_FILENO);
                    }
                    
                    // Cerrar todos los descriptores de pipe
                    for (int j = 0; j < command_count - 1; j++) {
                        close(pipes[j][0]);
                        close(pipes[j][1]);
                    }
                    
                    // Ejecutar comando
                    execvp(args_list[i][0], args_list[i]);
                    perror("execvp");
                    exit(EXIT_FAILURE);
                }
            }
            
            // Proceso padre - cerrar todos los pipes
            for (int i = 0; i < command_count - 1; i++) {
                close(pipes[i][0]);
                close(pipes[i][1]);
            }
            
            // Esperar a que todos los hijos terminen
            for (int i = 0; i < command_count; i++) {
                waitpid(pids[i], &status, 0);
            }
        }
    }
    return 0;
}