#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char **argv) {
    int start, pid, n;
    int buffer[1];

    if (argc != 4) {
        printf("Uso: anillo <n> <c> <s>\n");
        exit(1);
    }

    // Convertir argumentos y validar
    n = atoi(argv[1]);
    buffer[0] = atoi(argv[2]);
    start = atoi(argv[3]);

    // Validar n (mínimo 3 procesos)
    if (n < 3) {
        printf("Error: n debe ser >= 3\n");
        exit(1);
    }

    // Validar proceso inicial
    if (start < 1 || start > n) {
        printf("Error: s debe estar entre 1 y %d\n", n);
        exit(1);
    }

    printf("Se crearán %i procesos, se enviará el caracter %i desde proceso %i \n", n, buffer[0], start);

    // Pipes para comunicación
    int (*pipes)[2] = malloc(n * sizeof(int[2])); // Pipes entre hijos
    int padre_env[2];      // Pipe padre -> hijo inicial
    int padre_ret[2];      // Pipe hijo final -> padre

    // Crear pipes
    if (pipe(padre_env) < 0 || pipe(padre_ret) < 0) {
        perror("pipe");
        exit(1);
    }

    for (int i = 0; i < n; i++) {
        if (pipe(pipes[i]) < 0) {
            perror("pipe");
            exit(1);
        }
    }

    // Crear procesos hijos
    for (int i = 0; i < n; i++) {
        pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(1);
        } else if (pid == 0) {
            // HIJO i ---------------------------------------------------------
            int buf;
            int read_fd, write_fd;

            // Determinar entrada (lectura)
            if (i == start - 1) {
                read_fd = padre_env[0]; // Recibe del padre
            } else {
                read_fd = pipes[(i + n - 1) % n][0]; // Recibe del anterior
            }

            // Determinar salida (escritura)
            if (i == (start - 1 + n - 1) % n) {
                write_fd = padre_ret[1]; // Envía al padre (final del anillo)
            } else {
                write_fd = pipes[i][1]; // Envía al siguiente
            }

            // Cerrar pipes innecesarios
            for (int j = 0; j < n; j++) {
                // Cerrar todos los pipes del anillo excepto los usados
                if (pipes[j][0] != read_fd) close(pipes[j][0]);
                if (pipes[j][1] != write_fd) close(pipes[j][1]);
            }
            // Cerrar pipes del padre no utilizados
            close(padre_env[1]);
            close(padre_ret[0]);
            if (i != start - 1) close(padre_env[0]);
            if (i != (start - 1 + n - 1) % n) close(padre_ret[1]);

            // Leer, procesar y escribir
            read(read_fd, &buf, sizeof(int));
            buf++;
            write(write_fd, &buf, sizeof(int));

            // Cerrar descriptores usados
            close(read_fd);
            close(write_fd);
            free(pipes);
            exit(0);
        }
    }

    // PADRE ----------------------------------------------------------------
    // Cerrar extremos no usados
    close(padre_env[0]);
    close(padre_ret[1]);
    for (int i = 0; i < n; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
    free(pipes);

    // Enviar valor inicial al proceso de inicio
    write(padre_env[1], buffer, sizeof(int));
    close(padre_env[1]);

    // Recibir resultado final
    read(padre_ret[0], buffer, sizeof(int));
    close(padre_ret[0]);

    // Esperar a todos los hijos
    for (int i = 0; i < n; i++) {
        wait(NULL);
    }

    printf("Resultado final: %d\n", buffer[0]);
    return 0;
}