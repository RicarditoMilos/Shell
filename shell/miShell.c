#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h> // Para operaciones con archivos
#include <pwd.h> //para funciones que accedan a informacion del usuario, en este caso, las contrasegnas
#include <grp.h> //lo mismo que pwd pero con grupos

// Tamaño máximo para los comans
#define MAX_INPUT 1024
#define MAX_ARGS 64
#define BUFFER_SIZE 4096
#define USER_DATA_FILE "/usr/local/bin/usuarios_data.txt" //Aca se guardan los datos del ususario

// Estructura para datos de usuario
typedef struct {
    char nombre[64];
    char contrasena[64];
    char horario[64];  // Ejemplo: "09:00-17:00"
    char ips[128];     // Ejemplo: "192.168.1.1,localhost"
} Usuario;


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

//------------------------------------------------------------------------------//
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
//---------------------------------------------------------------------------------------//
//Funcion para mover archivos
void mover(const char *origen, const char *destino) {
    if (rename(origen, destino) == 0) {
        printf("'%s' se ha movido a '%s' correctamente.\n", origen, destino);
    } else {
        perror("Error al mover");
    }
}
//--------------------------------------------------------------------------------------//
//FUncion para renombrar archivos
void renombrar(const char *origen, const char *nuevo_nombre) {
    if (rename(origen, nuevo_nombre) == 0) {
        printf("'%s' se ha renombrado a '%s' correctamente.\n", origen, nuevo_nombre);
    } else {
        perror("Error al renombrar");
    }
}
//-------------------------------------------------------------------------------------//
//Funcion ls 
void listar(const char *directorio) {
    DIR *dir;
    struct dirent *entrada;

    // Abrir el directorio
    dir = opendir(directorio);
    if (dir == NULL) {
        perror("Error al abrir el directorio");
        return;
    }

    printf("Contenido de '%s':\n", directorio);
    while ((entrada = readdir(dir)) != NULL) {
        printf("%s\n", entrada->d_name);
    }

    closedir(dir);
}


//------------------------------------------------------------------------------------//
//Funcion para crear directorios
void creardir(const char *nombre_directorio) {
    // Intentar crear el directorio
    if (mkdir(nombre_directorio, 0755) == 0) {
        printf("Directorio '%s' creado exitosamente.\n", nombre_directorio);
    } else {
        perror("Error al crear el directorio");
    }
}

//-----------------------------------------------------------------------------------//
//Funcion para cambiar de directorio, cd
void ir(const char *ruta) {
    // Intentar cambiar el directorio
    if (chdir(ruta) == 0) {
        printf("Directorio cambiado a '%s'\n", ruta);
    } else {
        perror("Error al cambiar de directorio");
    }
}

//----------------------------------------------------------------------------------//
//funcion para cambiar los permisos de un archivo
void permisos(const char *modo, const char *archivo) {
    // Convertir el modo (en formato octal) a un entero
    int permisos = strtol(modo, NULL, 8);

    // Intentar cambiar los permisos del archivo
    if (chmod(archivo, permisos) == 0) {
        printf("Permisos de '%s' cambiados a %s\n", archivo, modo);
    } else {
        perror("Error al cambiar permisos");
    }
}


//----------------------------------------------------------------------------------//
//Funcion para cambiar los propietarios sobre un archivo o conjunto de archivos
void propietario(const char *usuario, const char *grupo, const char *archivo) {
    // Obtener el UID del usuario
    struct passwd *pwd = getpwnam(usuario);
    if (pwd == NULL) {
        printf("Usuario '%s' no encontrado.\n", usuario);
        return;
    }
    uid_t uid = pwd->pw_uid;

    // Obtener el GID del grupo
    gid_t gid = -1; // Si el grupo no es especificado, se ignora
    if (grupo != NULL) {
        struct group *grp = getgrnam(grupo);
        if (grp == NULL) {
            printf("Grupo '%s' no encontrado.\n", grupo);
            return;
        }
        gid = grp->gr_gid;
    }

    // Cambiar el propietario y grupo del archivo
    if (chown(archivo, uid, gid) == 0) {
        printf("Propietario de '%s' cambiado a usuario: '%s', grupo: '%s'\n",
               archivo, usuario, grupo ? grupo : "(sin cambios)");
    } else {
        perror("Error al cambiar propietario");
    }
}
//----------------------------------------------------------------------------------//
//Funcion para cambiar contrasena
void cambiar_contrasena() {
    const char *usuario = getenv("USER"); // Obtener el nombre del usuario actual
    if (usuario == NULL) {
        printf("No se pudo determinar el usuario actual.\n");
        return;
    }

    printf("Cambiando contrasena para el usuario '%s'.\n", usuario);

    // Crear un comando para llamar a 'passwd' con el usuario actual
    char comando[256];
    snprintf(comando, sizeof(comando), "passwd %s", usuario);

    // Ejecutar el comando
    int resultado = system(comando);
    if (resultado == 0) {
        printf("Contrasena cambiada exitosamente.\n");
    } else {
        printf("Error al cambiar la contrasena.\n");
    }
}
//----------------------------------------------------------------------------------//
//Funcion para agregar usuario con su ip y horario laboral
void agregar_usuario(const char *nombre, const char *horario, const char *ips) {
    FILE *archivo;
    char comando[256];

    // Crear el usuario en el sistema
    snprintf(comando, sizeof(comando), "useradd -m %s", nombre);
    if (system(comando) != 0) {
        printf("Error al crear el usuario '%s'.\n", nombre);
        return;
    }

    // Establecer contraseña
    printf("Estableciendo contraseña para '%s'.\n", nombre);
    snprintf(comando, sizeof(comando), "passwd %s", nombre);
    if (system(comando) != 0) {
        printf("Error al establecer la contraseña para '%s'.\n", nombre);
        return;
    }

    // Guardar los datos del usuario en el archivo
    archivo = fopen(USER_DATA_FILE, "a");
    if (archivo == NULL) {
        perror("Error al abrir el archivo de datos de usuarios");
        return;
    }

    fprintf(archivo, "Usuario: %s\n", nombre);
    fprintf(archivo, "Horario: %s\n", horario);
    fprintf(archivo, "IPs permitidas: %s\n", ips);
    fprintf(archivo, "-----------------------------------\n");

    fclose(archivo);

    printf("Usuario '%s' creado exitosamente con horario '%s' y acceso desde '%s'.\n",
           nombre, horario, ips);
}

//-----------------------------------------------------------------------------------//
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

        // Si el usuario escribe "copiar"
        if (strcmp(args[0], "copiar") == 0) {
            if (args[1] == NULL || args[2] == NULL) {
                printf("Uso: copiar <origen> <destino>\n");
            } else {
                copiar(args[1], args[2]);
            }
            continue;
        }

        if(strcmp(args[0],"mover") == 0){ //Si el usuario escribef "mover", ejecutar 
             if (args[1] == NULL || args[2] == NULL) {
                printf("Uso:<archivo_o_directorio_origen> <destino>\n");
            } else {
                mover(args[1], args[2]);
            }
        }


	//Funcion de renombrar
	if (strcmp(args[0], "renombrar") == 0) {
            if (args[1] == NULL || args[2] == NULL) {
                fprintf(stderr, "Uso: renombrar <archivo_o_directorio_origen> <nuevo_nombre>\n");
            } else {
                renombrar(args[1], args[2]);
            }
            continue;
        }


	//Funcion ls
	if (strcmp(args[0], "listar") == 0) {
    if (args[1] == NULL) {
        listar("."); //Listar el actual si no se especifica
    } else {
        listar(args[1]);
    }
    continue;
}

	//LLamada a funcion para crear directorio
if (strcmp(args[0], "creardir") == 0) {
    if (args[1] == NULL) {
        printf("Uso: creardir <nombre_directorio>\n");
    } else {
        creardir(args[1]);
    }
    continue;
}

//Llamada a funcion para cambiar permisos
if (strcmp(args[0], "permisos") == 0) {
    if (args[1] == NULL || args[2] == NULL) {
        printf("Uso: permisos <modo_octal> <archivo>\n");
    } else {
        permisos(args[1], args[2]);
    }
    continue;
}


//Lamada a funcion para cambiar propietario/s
if (strcmp(args[0], "propietario") == 0) {
    if (args[1] == NULL || args[2] == NULL || args[3] == NULL) {
        printf("Uso: propietario <usuario> <grupo> <archivo>\n");
    } else {
        propietario(args[1], args[2], args[3]);
    }
    continue;
}

// Llamada a funcion para cambiar la contrasena
if (strcmp(args[0], "contraseña") == 0) {
    cambiar_contrasena();
    continue;
}

//llamada a funcion para crear usuario
 
if (strcmp(args[0], "usuario") == 0) {
    if (args[1] == NULL || args[2] == NULL || args[3] == NULL) {
        printf("Uso: usuario <nombre> <horario> <ips>\n");
    } else {
        agregar_usuario(args[1], args[2], args[3]);
    }
    continue;
}








	}
}

int main() {
    // Iniciar la shell
    printf("Bienvenido a mi_shell. Escribe 'salir' para terminar.\n");
    shell_loop();
    return 0;
}

