
#include "pathname.h"
#include "directory.h"
#include "inode.h"
#include "diskimg.h"
#include "unixfilesystem.h" // Para acceder a fs->dfd

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h> // Para strtok y strdup


int pathname_lookup(struct unixfilesystem *fs, const char *pathname) {
    // Validar que la ruta sea absoluta (comienza con '/')
    if (pathname == NULL || pathname[0] != '/') {
        // fprintf(stderr, "pathname_lookup: Pathname '%s' is not absolute\n", pathname);
        return -1; // Ruta no absoluta o nula
    }

    // Si la ruta es solo "/", retornar el inodo raíz (inodo 1)
    if (strcmp(pathname, "/") == 0) {
        return ROOT_INUMBER;
    }

    // Copiar la ruta para poder modificarla con strtok
    char *pathname_copy = strdup(pathname);
    if (pathname_copy == NULL) {
        fprintf(stderr, "pathname_lookup: Error allocating memory for pathname copy\n");
        return -1; // Error de memoria
    }

    // El inodo actual comienza siendo el inodo raíz
    int current_inumber = ROOT_INUMBER;
    struct direntv6 entry; // Para almacenar la entrada de directorio encontrada

    // Usar strtok para dividir la ruta en componentes
    char *token = strtok(pathname_copy + 1, "/"); // Empezar después del '/' inicial

    while (token != NULL) {
        // Buscar el token actual en el directorio con inodo current_inumber
        int found = directory_findname(fs, token, current_inumber, &entry);

        if (found < 0) {
            // No se encontró el nombre en el directorio actual
            // fprintf(stderr, "pathname_lookup: Component '%s' not found in directory inode %d\n", token, current_inumber);
            free(pathname_copy); // Liberar la memoria antes de salir
            return -1; // Componente no encontrado
        }

        // Actualizar el inodo actual al inodo encontrado en la entrada de directorio
        current_inumber = entry.d_inumber;

        // Obtener el siguiente token
        token = strtok(NULL, "/");

        // Si hay más tokens, el inodo encontrado DEBE ser un directorio
        if (token != NULL) {
            struct inode next_inode;
            int err = inode_iget(fs, current_inumber, &next_inode);
            if (err < 0) {
                 fprintf(stderr, "pathname_lookup: Error getting inode %d for component '%s'\n", current_inumber, entry.d_name);
                 free(pathname_copy);
                 return -1; // Error al obtener el inodo
            }

            if (!((next_inode.i_mode & IALLOC) && ((next_inode.i_mode & IFMT) == IFDIR))) {
                 fprintf(stderr, "pathname_lookup: Component '%s' (inode %d) is not a directory\n", entry.d_name, current_inumber);
                 free(pathname_copy);
                 return -1; // No es un directorio intermedio
            }
        }
    }

    // Si el bucle termina, hemos llegado al final de la ruta.
    // current_inumber contiene el número de inodo del archivo o directorio final.
    free(pathname_copy); // Liberar la memoria
    return current_inumber; // Retornar el número de inodo final
}

