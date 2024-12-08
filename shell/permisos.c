#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

void cambiar_permisos(const char *modo, const char *archivo) {
    // Convertir el modo octal a entero
    mode_t permisos = strtol(modo, NULL, 8);

    // Intentar cambiar los permisos
    if (chmod(archivo, permisos) == 0) {
        printf("Permisos de '%s' cambiados a %s\n", archivo, modo);
    } else {
        perror("Error al cambiar los permisos");
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Uso: %s <modo_octales> <archivo1> [archivo2 ... archivoN]\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *modo = argv[1];

    // Cambiar permisos para cada archivo proporcionado
    for (int i = 2; i < argc; i++) {
        cambiar_permisos(modo, argv[i]);
    }

    return EXIT_SUCCESS;
}

