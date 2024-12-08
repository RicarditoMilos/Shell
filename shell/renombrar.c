#include <stdio.h>
#include <stdlib.h>

void renombrar(const char *nombre_actual, const char *nuevo_nombre) {
    if (rename(nombre_actual, nuevo_nombre) == 0) {
        printf("Archivo renombrado exitosamente de '%s' a '%s'.\n", nombre_actual, nuevo_nombre);
    } else {
        perror("Error al renombrar el archivo");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <nombre_actual> <nuevo_nombre>\n", argv[0]);
        return EXIT_FAILURE;
    }

    renombrar(argv[1], argv[2]);
    return EXIT_SUCCESS;
}

