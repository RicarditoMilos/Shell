#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void ir(const char *ruta) {
    // Intentar cambiar al directorio especificado
    if (chdir(ruta) == 0) {
        printf("Cambiado exitosamente al directorio: %s\n", ruta);
    } else {
        perror("Error al cambiar de directorio");
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <ruta_del_directorio>\n", argv[0]);
        return EXIT_FAILURE;
    }

    ir(argv[1]);
    return EXIT_SUCCESS;
}

