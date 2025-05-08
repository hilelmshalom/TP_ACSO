#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h> // Necesario para memcpy

#include "inode.h"
#include "diskimg.h"
#include "unixfilesystem.h" // Para acceder a fs->dfd y fs->superblock
#include "file.h" // Necesario para file_getblock en inode_indexlookup (para bloques indirectos)


int inode_iget(struct unixfilesystem *fs, int inumber, struct inode *inp) {
    // Validar el número de inodo. Los inodos válidos van de 1 a s_isize * sectores por bloque / tamaño del inodo
    // s_isize es el tamaño en bloques de la lista de inodos.
    // Cada bloque contiene DISKIMG_SECTOR_SIZE / sizeof(struct inode) inodos.
    int inodes_per_block = DISKIMG_SECTOR_SIZE / sizeof(struct inode);
    int max_inumber = fs->superblock.s_isize * inodes_per_block;

    if (inumber < 1 || inumber > max_inumber) {
        fprintf(stderr, "Error: Invalid inumber %d\n", inumber);
        return -1; // Inodo inválido
    }

    // Calcular el sector donde se encuentra el inodo.
    // Los inodos comienzan en INODE_START_SECTOR (sector 2).
    // El inodo 1 está en el primer sector de inodos.
    // El inodo 'inumber' está en el sector: INODE_START_SECTOR + (inumber - 1) / inodes_per_block
    int sector_num = INODE_START_SECTOR + (inumber - 1) / inodes_per_block;

    // Calcular el desplazamiento dentro del sector donde se encuentra el inodo.
    // El inodo 'inumber' es el (inumber - 1) % inodes_per_block-ésimo inodo en el sector.
    int offset_in_sector = ((inumber - 1) % inodes_per_block) * sizeof(struct inode);

    // Buffer para leer el sector completo
    char sector_buffer[DISKIMG_SECTOR_SIZE];

    // Leer el sector del disco.
    int bytes_read = diskimg_readsector(fs->dfd, sector_num, sector_buffer);
    if (bytes_read != DISKIMG_SECTOR_SIZE) {
        fprintf(stderr, "Error reading sector %d for inode %d\n", sector_num, inumber);
        return -1; // Error de lectura
    }

    // Copiar los datos del inodo desde el buffer del sector a la estructura de salida.
    memcpy(inp, sector_buffer + offset_in_sector, sizeof(struct inode));

    return 0; // Éxito
}

/**
 * TODO
 */
int inode_indexlookup(struct unixfilesystem *fs, struct inode *inp,
    int blockNum) {  

    // Obtener el tamaño del archivo en bytes
    int file_size = inode_getsize(inp);
    // Calcular el número total de bloques lógicos en el archivo
    int num_logical_blocks = (file_size + DISKIMG_SECTOR_SIZE - 1) / DISKIMG_SECTOR_SIZE;

    // Validar blockNum: debe ser un índice de bloque lógico válido dentro del archivo
    if (blockNum < 0 || blockNum >= num_logical_blocks) {
        // fprintf(stderr, "Error: Invalid block index %d for file size %d\n", blockNum, file_size);
        return -1; // Índice de bloque inválido
    }

    // Verificar si el archivo es grande (ILARG)
    if (inp->i_mode & ILARG) {
        // Archivo grande: usa punteros indirectos y doblemente indirectos

        // Los primeros 7 punteros (i_addr[0] a i_addr[6]) son a bloques indirectos.
        // El octavo puntero (i_addr[7]) es a un bloque doblemente indirecto.

        // Número de punteros por bloque indirecto (512 bytes / 2 bytes por puntero)
        int pointers_per_block = DISKIMG_SECTOR_SIZE / sizeof(uint16_t);

        // Bloques cubiertos por los 7 punteros indirectos directos
        int blocks_in_indirect_blocks = 7 * pointers_per_block;

        if (blockNum < blocks_in_indirect_blocks) {
            // El bloque lógico está en uno de los primeros 7 bloques indirectos.
            int indirect_block_index = blockNum / pointers_per_block; // Índice del puntero indirecto (0-6)
            int pointer_index_in_block = blockNum % pointers_per_block; // Índice del puntero dentro del bloque indirecto

            uint16_t indirect_block_sector = inp->i_addr[indirect_block_index];

            // Buffer para leer el bloque indirecto
            uint16_t indirect_buffer[pointers_per_block];

            // Leer el bloque indirecto
            int bytes_read = diskimg_readsector(fs->dfd, indirect_block_sector, indirect_buffer);
            if (bytes_read != DISKIMG_SECTOR_SIZE) {
                fprintf(stderr, "Error reading indirect block sector %d\n", indirect_block_sector);
                return -1; // Error de lectura
            }

            // El número de sector físico es el puntero en el bloque indirecto
            return indirect_buffer[pointer_index_in_block];

        } else {
            // El bloque lógico está en el bloque doblemente indirecto.
            // blockNum - blocks_in_indirect_blocks nos da el índice del bloque dentro del área doblemente indirecta.
            int double_indirect_block_index = blockNum - blocks_in_indirect_blocks;

            // El puntero i_addr[7] apunta al bloque doblemente indirecto.
            uint16_t double_indirect_sector = inp->i_addr[7];

            // Buffer para leer el bloque doblemente indirecto
            uint16_t double_indirect_buffer[pointers_per_block];

            // Leer el bloque doblemente indirecto
            int bytes_read = diskimg_readsector(fs->dfd, double_indirect_sector, double_indirect_buffer);
            if (bytes_read != DISKIMG_SECTOR_SIZE) {
                fprintf(stderr, "Error reading double indirect block sector %d\n", double_indirect_sector);
                return -1; // Error de lectura
            }

            // Calcular el índice del puntero indirecto dentro del bloque doblemente indirecto
            int indirect_pointer_index = double_indirect_block_index / pointers_per_block;

            // Validar que el índice del puntero indirecto sea válido
            if (indirect_pointer_index >= pointers_per_block) {
                 fprintf(stderr, "Error: Indirect pointer index %d out of bounds in double indirect block\n", indirect_pointer_index);
                 return -1; // Índice fuera de rango
            }

            // Obtener el sector del bloque indirecto del puntero doblemente indirecto
            uint16_t indirect_block_sector = double_indirect_buffer[indirect_pointer_index];

            // Calcular el índice del puntero dentro del bloque indirecto
            int pointer_index_in_block = double_indirect_block_index % pointers_per_block;

            // Buffer para leer el bloque indirecto referenciado
            uint16_t indirect_buffer[pointers_per_block];

            // Leer el bloque indirecto
            bytes_read = diskimg_readsector(fs->dfd, indirect_block_sector, indirect_buffer);
            if (bytes_read != DISKIMG_SECTOR_SIZE) {
                fprintf(stderr, "Error reading indirect block sector %d from double indirect\n", indirect_block_sector);
                return -1; // Error de lectura
            }

            // El número de sector físico es el puntero en el bloque indirecto
            return indirect_buffer[pointer_index_in_block];
        }

    } else {
        // Archivo pequeño: usa punteros directos
        // Los punteros i_addr[0] a i_addr[7] apuntan directamente a los bloques de datos.
        if (blockNum >= 8) {
            // Esto no debería ocurrir si blockNum fue validado contra el tamaño del archivo,
            // pero es una verificación de seguridad.
             fprintf(stderr, "Error: block index %d out of bounds for small file\n", blockNum);
            return -1; // Índice fuera de rango para archivo pequeño
        }
        return inp->i_addr[blockNum];
    }

    return -1; // Debería haber retornado antes, esto es un fallback en caso de lógica faltante
}

int inode_getsize(struct inode *inp) {
  return ((inp->i_size0 << 16) | inp->i_size1); 
}
