#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>

void cambiar_propietario(const char *propietario, const char *grupo, const char *archivo) {
    uid_t uid = -1;
    gid_t gid = -1;

    // Obtener UID del propietario
    if (propietario != NULL) {
        struct passwd *pwd = getpwnam(propietario);
        if (pwd == NULL) {
            fprintf(stderr, "Error: No se encontró el usuario '%s'.\n", propietario);
            exit(EXIT_FAILURE);
        }
        uid = pwd->pw_uid;
    }

    // Obtener GID del grupo
    if (grupo != NULL) {
        struct group *grp = getgrnam(grupo);
        if (grp == NULL) {
            fprintf(stderr, "Error: No se encontró el grupo '%s'.\n", grupo);
            exit(EXIT_FAILURE);
        }
        gid = grp->gr_gid;
    }

    // Cambiar propietario y/o grupo
    if (chown(archivo, uid, gid) == 0) {
        printf("Propietario y grupo de '%s' cambiados a '%s:%s'.\n",
               archivo, propietario ? propietario : "sin_cambio",
               grupo ? grupo : "sin_cambio");
    } else {
        perror("Error al cambiar propietario/grupo");
    }
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Uso: %s <propietario> <grupo> <archivo1> [archivo2 ... archivoN]\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *propietario = argv[1];
    const char *grupo = argv[2];

    // Cambiar propietario/grupo para cada archivo
    for (int i = 3; i < argc; i++) {
        cambiar_propietario(propietario, grupo, argv[i]);
    }

    return EXIT_SUCCESS;
}

