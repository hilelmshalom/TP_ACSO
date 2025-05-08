#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h> // Necesario para memcpy

#include "file.h"
#include "inode.h"
#include "diskimg.h"
#include "unixfilesystem.h" // Para acceder a fs->dfd


int file_getblock(struct unixfilesystem *fs, int inumber, int blockNum, void *buf) {
    struct inode in;

    // 1. Obtener el inodo del archivo
    int err = inode_iget(fs, inumber, &in);
    if (err < 0) {
        // fprintf(stderr, "file_getblock: Error getting inode %d\n", inumber);
        return -1; // Error al obtener el inodo
    }

    // Verificar si el inodo está asignado (aunque inode_iget ya debería manejar inodos inválidos)
    if (!(in.i_mode & IALLOC)) {
        // fprintf(stderr, "file_getblock: Inode %d is not allocated\n", inumber);
        return -1; // Inodo no asignado
    }

    // Obtener el tamaño del archivo en bytes
    int file_size = inode_getsize(&in);
    // Calcular el número total de bloques lógicos en el archivo
    int num_logical_blocks = (file_size + DISKIMG_SECTOR_SIZE - 1) / DISKIMG_SECTOR_SIZE;

    // 2. Validar blockNum: debe ser un índice de bloque lógico válido dentro del archivo
    if (blockNum < 0 || blockNum >= num_logical_blocks) {
        // fprintf(stderr, "file_getblock: Invalid block index %d for file size %d\n", blockNum, file_size);
        return -1; // Índice de bloque inválido
    }

    // 3. Obtener el número de sector físico correspondiente al blockNum lógico
    int physical_sector = inode_indexlookup(fs, &in, blockNum);
    if (physical_sector < 0) {
        // fprintf(stderr, "file_getblock: Error looking up block index %d for inode %d\n", blockNum, inumber);
        return -1; // Error al buscar el sector físico
    }

    // 4. Leer el sector físico del disco
    int bytes_read = diskimg_readsector(fs->dfd, physical_sector, buf);
    if (bytes_read != DISKIMG_SECTOR_SIZE) {
        fprintf(stderr, "file_getblock: Error reading physical sector %d\n", physical_sector);
        return -1; // Error de lectura del disco
    }

    // 5. Calcular el número de bytes válidos en este bloque
    // Para todos los bloques excepto el último, los bytes válidos son DISKIMG_SECTOR_SIZE.
    // Para el último bloque, son los bytes restantes del archivo.
    int bytes_valid_in_block;
    int offset_in_file = blockNum * DISKIMG_SECTOR_SIZE;

    if (offset_in_file + DISKIMG_SECTOR_SIZE <= file_size) {
        // Es un bloque completo
        bytes_valid_in_block = DISKIMG_SECTOR_SIZE;
    } else {
        // Es el último bloque y no está completo
        bytes_valid_in_block = file_size - offset_in_file;
    }

    return bytes_valid_in_block; // Retornar la cantidad de bytes válidos leídos
}

