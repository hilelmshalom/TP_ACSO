#include "directory.h"
#include "inode.h"
#include "diskimg.h"
#include "file.h"
#include "unixfilesystem.h" // Para acceder a fs->dfd
#include "direntv6.h" // Para la estructura direntv6

#ifndef DIRSIZ
#define DIRSIZ 14 // Define DIRSIZ if not already defined
#endif

#include <stdio.h>
#include <string.h>
#include <assert.h>

/**
 * TODO
 */
int directory_findname(struct unixfilesystem *fs, const char *name,
		int dirinumber, struct direntv6 *dirEnt) {
      struct inode dir_inode;

      // 1. Obtener el inodo del directorio padre
      int err = inode_iget(fs, dirinumber, &dir_inode);
      if (err < 0) {
          // fprintf(stderr, "directory_findname: Error getting directory inode %d\n", dirinumber);
          return -1; // Error al obtener el inodo
      }
  
      // 2. Verificar si el inodo es realmente un directorio
      if (!((dir_inode.i_mode & IALLOC) && ((dir_inode.i_mode & IFMT) == IFDIR))) {
          // fprintf(stderr, "directory_findname: Inode %d is not a valid directory\n", dirinumber);
          return -1; // No es un directorio válido
      }
  
      // Obtener el tamaño del directorio en bytes
      int dir_size = inode_getsize(&dir_inode);
      // Calcular el número total de bloques lógicos en el directorio
      int num_logical_blocks = (dir_size + DISKIMG_SECTOR_SIZE - 1) / DISKIMG_SECTOR_SIZE;
  
      // Tamaño de una entrada de directorio
      int dirent_size = sizeof(struct direntv6);
      // Número de entradas de directorio por bloque
      int dirents_per_block = DISKIMG_SECTOR_SIZE / dirent_size;
  
      // Buffer para leer un bloque del directorio
      char block_buffer[DISKIMG_SECTOR_SIZE];
  
      // 3. Iterar sobre los bloques del directorio
      for (int bno = 0; bno < num_logical_blocks; bno++) {
          // Leer el bloque actual del directorio
          int bytes_read = file_getblock(fs, dirinumber, bno, block_buffer);
          if (bytes_read < 0) {
              fprintf(stderr, "directory_findname: Error reading block %d of directory inode %d\n", bno, dirinumber);
              return -1; // Error de lectura del bloque
          }
  
          // 4. Iterar sobre las entradas de directorio dentro del bloque
          // El número de entradas válidas en el bloque es bytes_read / dirent_size
          int num_valid_dirents_in_block = bytes_read / dirent_size;
  
          for (int i = 0; i < num_valid_dirents_in_block; i++) {
              // Obtener un puntero a la entrada de directorio actual en el buffer
              struct direntv6 *current_dirent = (struct direntv6 *)(block_buffer + i * dirent_size);
  
              // 5. Comparar el nombre buscado con el nombre en la entrada de directorio actual
              // Los nombres en direntv6.d_name tienen tamaño fijo (14 bytes) y pueden estar rellenados con nulos.
              // strncmp es útil aquí para comparar hasta el tamaño máximo del nombre.
              // También debemos asegurarnos de que el nombre en la entrada no sea solo nulos (entrada no utilizada).
              if (current_dirent->d_inumber != 0 && strncmp(current_dirent->d_name, name, DIRSIZ) == 0) {
                  // Se encontró una coincidencia. Copiar la entrada encontrada a dirEnt.
                  memcpy(dirEnt, current_dirent, sizeof(struct direntv6));
                  return 0; // Éxito, nombre encontrado
              }
          }
      }
  
      // Si llegamos aquí, el nombre no fue encontrado en el directorio.
      fprintf(stderr, "directory_findname: Name '%s' not found in directory inode %d\n", name, dirinumber);
      return -1; // Nombre no encontrado
}
