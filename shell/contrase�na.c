#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

void cambiar_contrasena(const char *usuario) {
    if (usuario == NULL) {
        fprintf(stderr, "Error: Usuario no especificado.\n");
        exit(EXIT_FAILURE);
    }

    printf("Cambiando la contraseña para el usuario '%s'. Necesitarás permisos de superusuario.\n", usuario);

    // Invocar el comando passwd con execlp
    if (execlp("passwd", "passwd", usuario, NULL) == -1) {
        perror("Error al ejecutar passwd");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <usuario>\n", argv[0]);
        return EXIT_FAILURE;
    }

    cambiar_contrasena(argv[1]);
    return EXIT_SUCCESS;
}

