#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>

void listar(const char *ruta) {
    DIR *directorio;
    struct dirent *entrada;

    // Abrir el directorio
    directorio = opendir(ruta);
    if (directorio == NULL) {
        fprintf(stderr, "Error al abrir el directorio '%s': %s\n", ruta, strerror(errno));
        exit(EXIT_FAILURE);
    }

    printf("Contenido del directorio '%s':\n", ruta);
    
    // Leer las entradas del directorio
    while ((entrada = readdir(directorio)) != NULL) {
        printf("  %s\n", entrada->d_name);
    }

    // Cerrar el directorio
    closedir(directorio);
}

int main(int argc, char *argv[]) {
    const char *ruta;

    if (argc == 1) {
        ruta = "."; // Si no se especifica ruta, se usa el directorio actual
    } else if (argc == 2) {
        ruta = argv[1];
    } else {
        fprintf(stderr, "Uso: %s [ruta_del_directorio]\n", argv[0]);
        return EXIT_FAILURE;
    }

    listar(ruta);
    return EXIT_SUCCESS;
}

