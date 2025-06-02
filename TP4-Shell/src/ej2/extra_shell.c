#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <ctype.h>

#define MAX_COMMANDS 200
#define MAX_ARGS 256
#define MAX_PIPES (MAX_COMMANDS - 1)

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
                if (arg_count < MAX_ARGS - 1) {
                    args[arg_count++] = start;
                }
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
                if (arg_count < MAX_ARGS - 1) {
                    args[arg_count++] = start;
                }
                start = NULL;
            }
        }
        else if (!in_quotes && *ptr == '\\' && *(ptr+1) != '\0') {
            if (start == NULL) start = ptr;
            *ptr = *(ptr+1);
            memmove(ptr+1, ptr+2, strlen(ptr+2)+1);
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

    if (start != NULL && arg_count < MAX_ARGS - 1) {
        args[arg_count++] = start;
    }
    
    // Manejar argumentos en blanco como cadenas vacías
    for (int i = 0; i < arg_count; i++) {
        if (args[i][0] == '\0') {
            args[i] = strdup("");
        }
    }
    
    args[arg_count] = NULL;
}

int validate_pipeline(char **commands, int command_count) {
    if (command_count == 0) return 0;
    
    // Verificar pipe al inicio
    if (strlen(commands[0]) == 0) {
        fprintf(stderr, "Error: Pipe al inicio\n");
        return 0;
    }
    
    // Verificar pipe al final
    if (strlen(commands[command_count-1]) == 0) {
        fprintf(stderr, "Error: Pipe al final\n");
        return 0;
    }
    
    // Verificar pipes consecutivos o vacíos
    for (int i = 0; i < command_count; i++) {
        if (strlen(commands[i]) == 0) {
            fprintf(stderr, "Error: Pipe vacío\n");
            return 0;
        }
    }
    
    return 1;
}

int is_exit_command(char **args) {
    return args[0] && strcmp(args[0], "exit") == 0 && args[1] == NULL;
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

        // Procesamiento especial para exit
        if (strcmp(command, "exit") == 0) {
            break;
        }

        // Reset command processing
        command_count = 0;
        char *token = strtok(command, "|");
        while (token != NULL && command_count < MAX_COMMANDS) {
            // Trim espacios iniciales/finales
            while (*token == ' ') token++;
            char *end = token + strlen(token) - 1;
            while (end > token && *end == ' ') end--;
            *(end + 1) = '\0';
            
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
        int has_exit = 0;
        
        for (int i = 0; i < command_count; i++) {
            parse_command(commands[i], args_list[i]);
            
            if (args_list[i][0] == NULL) {
                command_count = 0;
                break;
            }
            
            // Detectar comandos exit en pipelines
            if (is_exit_command(args_list[i])) {
                has_exit = 1;
            }
        }
        
        if (command_count == 0) {
            continue;
        }

        // Manejar exit en pipelines
        if (has_exit) {
            fprintf(stderr, "Error: 'exit' no puede usarse en pipelines\n");
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
                int status;
                waitpid(pid, &status, 0);
            }
        } 
        // Múltiples comandos con pipes
        else {
            int pipes[MAX_PIPES][2];
            pid_t pids[command_count];
            
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
                        wait(NULL);
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
                int status;
                waitpid(pids[i], &status, 0);
            }
        }
    }
    return 0;
}