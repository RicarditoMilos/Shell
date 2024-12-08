#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define BUFFER_SIZE 4096

void copiar(const char *origen, const char *destino) {
    int fd_origen, fd_destino;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_leidos, bytes_escritos;

    // Abrir el archivo origen en modo de solo lectura
    fd_origen = open(origen, O_RDONLY);
    if (fd_origen == -1) {
        fprintf(stderr, "Error al abrir el archivo origen '%s': %s\n", origen, strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Crear/abrir el archivo destino en modo escritura, con permisos 0644
    fd_destino = open(destino, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd_destino == -1) {
        fprintf(stderr, "Error al crear el archivo destino '%s': %s\n", destino, strerror(errno));
        close(fd_origen);
        exit(EXIT_FAILURE);
    }

    // Leer del archivo origen y escribir en el archivo destino
    while ((bytes_leidos = read(fd_origen, buffer, BUFFER_SIZE)) > 0) {
        bytes_escritos = write(fd_destino, buffer, bytes_leidos);
        if (bytes_escritos != bytes_leidos) {
            fprintf(stderr, "Error al escribir en el archivo destino '%s': %s\n", destino, strerror(errno));
            close(fd_origen);
            close(fd_destino);
            exit(EXIT_FAILURE);
        }
    }

    if (bytes_leidos == -1) {
        fprintf(stderr, "Error al leer del archivo origen '%s': %s\n", origen, strerror(errno));
    }

    // Cerrar los archivos
    close(fd_origen);
    close(fd_destino);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <archivo_origen> <archivo_destino>\n", argv[0]);
        return EXIT_FAILURE;
    }

    copiar(argv[1], argv[2]);
    printf("Archivo copiado exitosamente de '%s' a '%s'\n", argv[1], argv[2]);
    return EXIT_SUCCESS;
}

