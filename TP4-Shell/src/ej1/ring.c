#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char **argv)
{
	int start, pid, n;
	int buffer[1];

	if (argc != 4){ printf("Uso: anillo <n> <c> <s> \n"); exit(0);}

	/* Parsing of arguments */
	n = atoi(argv[1]);
	buffer[0] = atoi(argv[2]);
	start = atoi(argv[3]);

	printf("Se crearán %i procesos, se enviará el caracter %i desde proceso %i \n", n, buffer[0], start);

	/* You should start programming from here... */

	int pipes[n][2];       // Pipes entre hijos
	int padre_env[2];      // Pipe de padre → hijo[start]
	int padre_ret[2];      // Pipe de hijo[prev] → padre

	pipe(padre_env);
	pipe(padre_ret);

	for (int i = 0; i < n; i++) {
		if (pipe(pipes[i]) < 0) {
			perror("pipe");
			exit(1);
		}
	}

	for (int i = 0; i < n; i++) {
		if ((pid = fork()) == 0) {
			int buf;
			int read_fd, write_fd;

			// Desde padre
			if (i == start) {
				read_fd = padre_env[0];
			} else {
				read_fd = pipes[(i + n - 1) % n][0];
			}

			// Hacia padre
			if (i == (start + n - 1) % n) {
				write_fd = padre_ret[1];
			} else {
				write_fd = pipes[i][1];
			}

			// Cerrar pipes innecesarios
			for (int j = 0; j < n; j++) {
				if (pipes[j][0] != read_fd) close(pipes[j][0]);
				if (pipes[j][1] != write_fd) close(pipes[j][1]);
			}
			if (read_fd != padre_env[0]) close(padre_env[0]);
			if (write_fd != padre_ret[1]) close(padre_ret[1]);
			close(padre_env[1]);
			close(padre_ret[0]);

			read(read_fd, &buf, sizeof(int));
			buf++;
			write(write_fd, &buf, sizeof(int));

			close(read_fd);
			close(write_fd);
			exit(0);
		}
	}

	// Padre
	close(padre_env[0]);
	close(padre_ret[1]);

	write(padre_env[1], buffer, sizeof(int));
	close(padre_env[1]);

	read(padre_ret[0], buffer, sizeof(int));
	close(padre_ret[0]);

	for (int i = 0; i < n; i++) wait(NULL);

	printf("Resultado final: %d\n", buffer[0]);
	return 0;
}
