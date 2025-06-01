#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

#define MAX_COMMANDS 200
#define MAX_ARGS 256

void parse_command(char *input, char **args) {
    int arg_count = 0;
    char *token = strtok(input, " ");
    while (token != NULL && arg_count < MAX_ARGS - 1) {
        args[arg_count++] = token;
        token = strtok(NULL, " ");
    }
    args[arg_count] = NULL;
}

int main() {
    char command[256];
    char *commands[MAX_COMMANDS];
    int command_count = 0;

    while (1) {
        if (isatty(STDIN_FILENO)) {
            printf("Shell> ");
            fflush(stdout);
        }

        // Leer comando del usuario
        if (!fgets(command, sizeof(command), stdin)) {
            printf("\n");
            break; // Salir con Ctrl+D
        }
        command[strcspn(command, "\n")] = '\0';

        // Comprobar si es comando de salida
        if (strcmp(command, "exit") == 0) {
            break;
        }

        // Tokenizar por pipes
        command_count = 0;
        char *token = strtok(command, "|");
        while (token != NULL && command_count < MAX_COMMANDS) {
            // Eliminar espacios en blanco iniciales
            while (*token == ' ') token++;
            commands[command_count++] = token;
            token = strtok(NULL, "|");
        }

        // Si no hay comandos, continuar
        if (command_count == 0) {
            continue;
        }

        // Parsear cada comando en argumentos
        char *args_list[MAX_COMMANDS][MAX_ARGS];
        for (int i = 0; i < command_count; i++) {
            parse_command(commands[i], args_list[i]);
        }

        // Caso: comando único (sin pipes)
        if (command_count == 1) {
            pid_t pid = fork();
            if (pid == 0) {
                // Proceso hijo
                execvp(args_list[0][0], args_list[0]);
                perror("execvp");
                exit(EXIT_FAILURE);
            } else if (pid < 0) {
                perror("fork");
            } else {
                // Proceso padre espera al hijo
                waitpid(pid, NULL, 0);
            }
        } 
        // Caso: múltiples comandos con pipes
        else {
            int pipes[command_count-1][2];
            pid_t pids[command_count];
            int num_children = command_count;

            // Crear pipes
            for (int i = 0; i < command_count - 1; i++) {
                if (pipe(pipes[i]) == -1) {
                    perror("pipe");
                    num_children = 0;
                    break;
                }
            }

            // Crear procesos hijos
            for (int i = 0; i < command_count; i++) {
                pids[i] = fork();
                
                if (pids[i] < 0) {
                    perror("fork");
                    num_children = i;
                    break;
                }
                
                if (pids[i] == 0) { // Proceso hijo
                    // Conectar entrada del pipe anterior (si no es el primero)
                    if (i > 0) {
                        dup2(pipes[i-1][0], STDIN_FILENO);
                    }
                    
                    // Conectar salida al pipe siguiente (si no es el último)
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

            // Proceso padre: cerrar todos los pipes
            for (int i = 0; i < command_count - 1; i++) {
                close(pipes[i][0]);
                close(pipes[i][1]);
            }

            // Esperar a todos los hijos
            for (int i = 0; i < num_children; i++) {
                waitpid(pids[i], NULL, 0);
            }
        }
    }
    return 0;
}