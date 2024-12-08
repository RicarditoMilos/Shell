#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

void creardir(const char *nombre_directorio) {
    // Intentar crear el directorio con permisos 0755
    if (mkdir(nombre_directorio, 0755) == 0) {
        printf("Directorio '%s' creado exitosamente.\n", nombre_directorio);
    } else {
        perror("Error al crear el directorio");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <nombre_directorio>\n", argv[0]);
        return EXIT_FAILURE;
    }

    creardir(argv[1]);
    return EXIT_SUCCESS;
}

