#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h> // Para operaciones con archivos

// Tamaño máximo para los comandos
#define MAX_INPUT 1024
#define MAX_ARGS 64
#define BUFFER_SIZE 4096

// Función para dividir el comando en argumentos
void parse_command(char *input, char **args) {
    char *token;
    int i = 0;

    // Dividir el input por espacios
    token = strtok(input, " \n");
    while (token != NULL) {
        args[i++] = token;
        token = strtok(NULL, " \n");
    }
    args[i] = NULL; // Terminar la lista de argumentos
}

// Función para copiar archivos
void copiar(const char *origen, const char *destino) {
    int src_fd, dest_fd;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read, bytes_written;

    // Abrir archivo de origen
    src_fd = open(origen, O_RDONLY);
    if (src_fd < 0) {
        perror("Error al abrir el archivo de origen");
        return;
    }

    // Crear archivo de destino
    dest_fd = open(destino, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dest_fd < 0) {
        perror("Error al crear el archivo de destino");
        close(src_fd);
        return;
    }

    // Copiar contenido
    while ((bytes_read = read(src_fd, buffer, BUFFER_SIZE)) > 0) {
        bytes_written = write(dest_fd, buffer, bytes_read);
        if (bytes_written != bytes_read) {
            perror("Error al escribir en el archivo de destino");
            close(src_fd);
            close(dest_fd);
            return;
        }
    }

    if (bytes_read < 0) {
        perror("Error al leer el archivo de origen");
    }

    // Cerrar archivos
    close(src_fd);
    close(dest_fd);

    printf("Archivo copiado de '%s' a '%s'.\n", origen, destino);
}

void mover(const char *origen, const char *destino) {
    if (rename(origen, destino) == 0) {
        printf("'%s' se ha movido a '%s' correctamente.\n", origen, destino);
    } else {
        perror("Error al mover");
    }
}

// Función principal del loop de la shell
void shell_loop() {
    char input[MAX_INPUT];
    char *args[MAX_ARGS];
    pid_t pid;

    while (1) {
        // Mostrar el prompt
        printf("mi_shell> ");
        fflush(stdout);

        // Leer el comando del usuario
        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("\nSaliendo de la shell.\n");
            break;
        }

        // Parsear el comando
        parse_command(input, args);

        // Si no hay comando, continuar
        if (args[0] == NULL) {
            continue;
        }

        // Si el usuario escribe "salir", terminar la shell
        if (strcmp(args[0], "salir") == 0) {
            printf("Saliendo de la shell.\n");
            break;
        }

        // Si el usuario escribe "copiar", manejarlo directamente
        if (strcmp(args[0], "copiar") == 0) {
            if (args[1] == NULL || args[2] == NULL) {
                printf("Uso: copiar <origen> <destino>\n");
            } else {
                copiar(args[1], args[2]);
            }
            continue;
        }

        if(strcmp(args[0],"mover") == 0){
             if (args[1] == NULL || args[2] == NULL) {
                printf("Uso:<archivo_o_directorio_origen> <destino>\n");
            } else {
                mover(args[1], args[2]);
            }
        }
    }
}

int main() {
    // Iniciar la shell
    printf("Bienvenido a mi_shell. Escribe 'salir' para terminar.\n");
    shell_loop();
    return 0;
}

