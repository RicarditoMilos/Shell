#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h> // Para operaciones con archivos
#include <pwd.h> //para funciones que accedan a informacion del usuario, en este caso, las contrasegnas
#include <grp.h> //lo mismo que pwd pero con grupos
#include <errno.h>
#include <time.h> // Para timestamps

// Tamano maximo para los comdos
#undef MAX_INPUT // Eliminar la definición previa si existe
#define MAX_INPUT 1024
#define MAX_ARGS 64
#define BUFFER_SIZE 4096
#define USER_DATA_FILE "/usr/local/bin/usuarios_data.txt" //Aca se guardan los datos del ususario
#define HISTORIAL_FILE "historial.log" // Archivo para el historial
#define ERROR_LOG_FILE "/var/log/shell/sistema_error.log" // Archivo para errores
// Estructura para datos de usuario
typedef struct {
    char nombre[64];
    char contrasena[64];
    char horario[64];  // Ejemplo: "09:00-17:00"
    char ips[128];     // Ejemplo: "192.168.1.1,localhost"
} Usuario;

// Funcion para obtener el timestamp actual
void obtener_timestamp(char *buffer, size_t size) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    if (t != NULL) {
        strftime(buffer, size, "%Y-%m-%d %H:%M:%S", t);
    } else {
        snprintf(buffer, size, "Timestamp no disponible");
    }
}

// Funcion para registrar en el historial
void registrar_historial(const char *comando) {
    FILE *archivo = fopen(HISTORIAL_FILE, "a");
    if (archivo == NULL) {
        perror("Error al abrir historial.log");
        return;
    }

    char timestamp[64];
    obtener_timestamp(timestamp, sizeof(timestamp));

    fprintf(archivo, "%s: %s\n", timestamp, comando);
    fclose(archivo);
}

// Funcion para registrar errores
void registrar_error(const char *mensaje) {
    FILE *archivo = fopen(ERROR_LOG_FILE, "a");
    if (archivo == NULL) {
        perror("Error al abrir sistema_error.log");
        return;
    }

    char timestamp[64];
    obtener_timestamp(timestamp, sizeof(timestamp));
    perror(mensaje);
    fprintf(archivo, "%s: ERROR: %s\n", timestamp, mensaje);
    fclose(archivo);
}

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
// Función para copiar archivoso directorios
void copiar_archivo(const char *origen, const char *destino) {
    int src_fd, dest_fd;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read, bytes_written;

    src_fd = open(origen, O_RDONLY);
    if (src_fd < 0) {
        registrar_error("Error al abrir el archivo de origen");
        return;
    }

    dest_fd = open(destino, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dest_fd < 0) {
        registrar_error("Error al crear el archivo de destino");
        close(src_fd);
        return;
    }

    while ((bytes_read = read(src_fd, buffer, BUFFER_SIZE)) > 0) {
        bytes_written = write(dest_fd, buffer, bytes_read);
        if (bytes_written != bytes_read) {
            registrar_error("Error al escribir en el archivo de destino");
            close(src_fd);
            close(dest_fd);
            return;
        }
    }

    if (bytes_read < 0) {
        registrar_error("Error al leer el archivo de origen");
    }

    close(src_fd);
    close(dest_fd);

    printf("Archivo copiado de '%s' a '%s'.\n", origen, destino);
}

// Función para copiar un directorio de forma recursiva
void copiar_directorio(const char *origen, const char *destino) {
    DIR *dir;
    struct dirent *entry;
    struct stat statbuf;
    char src_path[PATH_MAX];
    char dest_path[PATH_MAX];

    if (mkdir(destino, 0755) < 0 && errno != EEXIST) {
        registrar_error("Error al crear el directorio de destino");
        return;
    }

    dir = opendir(origen);
    if (dir == NULL) {
        registrar_error("Error al abrir el directorio de origen");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        snprintf(src_path, sizeof(src_path), "%s/%s", origen, entry->d_name);
        snprintf(dest_path, sizeof(dest_path), "%s/%s", destino, entry->d_name);

        if (stat(src_path, &statbuf) == 0) {
            if (S_ISDIR(statbuf.st_mode)) {
                // Es un directorio: copiar recursivamente
                copiar_directorio(src_path, dest_path);
            } else if (S_ISREG(statbuf.st_mode)) {
                // Es un archivo: copiar
                copiar_archivo(src_path, dest_path);
            }
        }
    }

    closedir(dir);
    printf("Directorio copiado de '%s' a '%s'.\n", origen, destino);
}

// Función para decidir si se copia un archvo o un directorio
void copiar(const char *origen, const char *destino) {
    struct stat statbuf;

    if (stat(origen, &statbuf) < 0) {
        registrar_error("Error al obtener información del origen");
        return;
    }

    if (S_ISDIR(statbuf.st_mode)) {
        // Es un directorio
        copiar_directorio(origen, destino);
    } else if (S_ISREG(statbuf.st_mode)) {
        // Es un archivo
        copiar_archivo(origen, destino);
    } else {
        printf("El origen '%s' no es un archivo ni un directorio válido.\n", origen);
    }
}
//---------------------------------------------------------------------------------------//
//Funcion de ir
void cambiar_directorio(const char *ruta) {
    // Intentar cambiar al directorio especificado
    if (chdir(ruta) == 0) {
        char cwd[PATH_MAX];

        // Obtener y mostrar el nuevo directorio actual
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("Directorio cambiado a: %s\n", cwd);
        } else {
            registrar_error("Error al obtener el directorio actual");
        }
    } else {
        registrar_error("Error al cambiar de directorio");
    }
}

//---------------------------------------------------------------------------------------//
//Funcion para mover archivos o directorio
void mover(const char *origen, const char *destino) {
    struct stat statbuf;

    // Verificar si el destino es un directorio
    if (stat(destino, &statbuf) == 0 && S_ISDIR(statbuf.st_mode)) {
        char nuevo_destino[PATH_MAX];
        snprintf(nuevo_destino, sizeof(nuevo_destino), "%s/%s", destino, strrchr(origen, '/') ? strrchr(origen, '/') + 1 : origen);

        if (rename(origen, nuevo_destino) == 0) {
            printf("'%s' se ha movido a '%s' correctamente.\n", origen, nuevo_destino);
        } else {
            registrar_error("Error al mover");
        }
    } else {
        // Si el destino no es un directorio, mover directamente
        if (rename(origen, destino) == 0) {
            printf("'%s' se ha movido a '%s' correctamente.\n", origen, destino);
        } else {
            registrar_error("Error al mover");
        }
    }
}
//--------------------------------------------------------------------------------------//
//FUncion para renombrar archivos
void renombrar(const char *origen, const char *nuevo_nombre) {
    if (rename(origen, nuevo_nombre) == 0) {
        printf("'%s' se ha renombrado a '%s' correctamente.\n", origen, nuevo_nombre);
    } else {
        registrar_error("Error al renombrar");
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
        registrar_error("Error al abrir el directorio");
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
        registrar_error("Error al crear el directorio");
    }
}

//-----------------------------------------------------------------------------------//
//Funcion para cambiar de directorio, cd
void ir(const char *ruta) {
    // Intentar cambiar el directorio
    if (chdir(ruta) == 0) {
        printf("Directorio cambiado a '%s'\n", ruta);
    } else {
        registrar_error("Error al cambiar de directorio");
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
        registrar_error("Error al cambiar permisos");
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
        registrar_error("Error al cambiar propietario");
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
        registrar_error("Error al abrir el archivo de datos de usuarios");
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
//------------------------------------------------------------------------------//
// Función para levantar y apagar demonios
void gestionar_demonio(const char *nombre_demonio, const char *accion) {
    if (strcmp(accion, "iniciar") == 0 || strcmp(accion, "detener") == 0) {
        printf("Intentando %s el demonio '%s'\n", accion, nombre_demonio);
        pid_t pid = fork();
        if (pid == 0) {
            // Proceso hijo para gestionar demonio
            const char *argv[] = {"/usr/bin/systemctl", accion, nombre_demonio, NULL};
            execvp(argv[0], (char * const *)argv);
            perror("Error al gestionar el demonio");
            exit(EXIT_FAILURE);
        } else if (pid > 0) {
            // Proceso padre espera
            waitpid(pid, NULL, 0);
            printf("Demonio '%s' %s con éxito.\n", nombre_demonio, accion);
        } else {
            registrar_error("Error al gestionar demonio");
        }
    } else {
        printf("Acción no válida: use 'iniciar' o 'detener'.\n");
    }
}

//------------------------------------------------------------------------------//
// Función para ejecutar comandos genéricos
void ejecutar_comando(const char *comando) {
    pid_t pid = fork();
    if (pid == 0) {
        // Proceso hijo para ejecutar el comando
        char *argv[] = {"/bin/sh", "-c", (char *)comando, NULL};
        execvp(argv[0], argv);
        perror("Error al ejecutar el comando");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        // Proceso padre espera
        waitpid(pid, NULL, 0);
        printf("Comando ejecutado: %s\n", comando);
    } else {
        registrar_error("Error al ejecutar comando genérico");
    }
}

//------------------------------------------------------------------------------//
// Función para registrar inicio y cierre de sesión
void registrar_sesion(const char *usuario, const char *accion) {
    char log_path[PATH_MAX] = "usuario_horarios_log";
    FILE *archivo = fopen(log_path, "a");
    if (archivo == NULL) {
        registrar_error("Error al abrir usuario_horarios_log");
        return;
    }

    char timestamp[64];
    obtener_timestamp(timestamp, sizeof(timestamp));

    fprintf(archivo, "%s: Usuario '%s' %s sesión\n", timestamp, usuario, accion);
    fclose(archivo);
}

//------------------------------------------------------------------------------//
// Función para transferencias por SCP o FTP
void transferencia_archivo(const char *origen, const char *destino, const char *metodo) {
    char log_path[PATH_MAX] = "Shell_transferencias.log";
    FILE *archivo = fopen(log_path, "a");
    if (archivo == NULL) {
        registrar_error("Error al abrir Shell_transferencias.log");
        return;
    }

    char timestamp[64];
    obtener_timestamp(timestamp, sizeof(timestamp));

    if (strcmp(metodo, "scp") == 0 || strcmp(metodo, "ftp") == 0) {
        fprintf(archivo, "%s: Transferencia iniciada de '%s' a '%s' usando %s\n", timestamp, origen, destino, metodo);
        fclose(archivo);
        ejecutar_comando(metodo);  // Ejemplo para delegar en herramientas como SCP
    } else {
        fprintf(archivo, "%s: Método de transferencia no soportado: '%s'\n", timestamp, metodo);
        fclose(archivo);
    }
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
            registrar_historial(args[0]);
            printf("Saliendo de la shell.\n");
            break;
        }

        // Si el usuario escribe "copiar"
        if (strcmp(args[0], "copiar") == 0) {
            if (args[1] == NULL || args[2] == NULL) {
                registrar_historial(args[1]);
                printf("Uso: copiar <origen> <destino>\n");
            } else {
                registrar_historial(args[0]);
                copiar(args[1], args[2]);
            }
            continue;
        }

        if(strcmp(args[0],"mover") == 0){ //Si el usuario escribef "mover", ejecutar 
             if (args[1] == NULL || args[2] == NULL) {
                printf("Uso:<archivo_o_directorio_origen> <destino>\n");
            } else {
                registrar_historial(args[0]);
                mover(args[1], args[2]);
            }
        }


	//Funcion de renombrar
	if (strcmp(args[0], "renombrar") == 0) {
            if (args[1] == NULL || args[2] == NULL) {
                fprintf(stderr, "Uso: renombrar <archivo_o_directorio_origen> <nuevo_nombre>\n");
            } else {
                registrar_historial(args[0]);
                renombrar(args[1], args[2]);
            }
            continue;
        }


	//Funcion ls
	if (strcmp(args[0], "listar") == 0) {
    if (args[1] == NULL) {
        registrar_historial(args[0]);
        listar("."); //Listar el actual si no se especifica
    } else {
        registrar_historial(args[0]);
        listar(args[1]);
    }
    continue;
}

	//LLamada a funcion para crear directorio
if (strcmp(args[0], "creardir") == 0) {
    if (args[1] == NULL) {
        printf("Uso: creardir <nombre_directorio>\n");
    } else {
        registrar_historial(args[0]);
        creardir(args[1]);
    }
    continue;
}

//Llamada a funcion para cambiar permisos
if (strcmp(args[0], "permisos") == 0) {
    if (args[1] == NULL || args[2] == NULL) {
        printf("Uso: permisos <modo_octal> <archivo>\n");
    } else {
        registrar_historial(args[0]);
        permisos(args[1], args[2]);
    }
    continue;
}


//Lamada a funcion para cambiar propietario/s
if (strcmp(args[0], "propietario") == 0) {
    if (args[1] == NULL || args[2] == NULL || args[3] == NULL) {
        printf("Uso: propietario <usuario> <grupo> <archivo>\n");
    } else {
        registrar_historial(args[0]);
        propietario(args[1], args[2], args[3]);
    }
    continue;
}

// Llamada a funcion para cambiar la contrasena
if (strcmp(args[0], "contraseña") == 0) {
    registrar_historial(args[0]);
    cambiar_contrasena();
    continue;
}
// Llamada a función para gestionar demonios
if (strcmp(args[0], "gestionar_demonio") == 0) {
    if (args[1] == NULL || args[2] == NULL) {
        printf("Uso: gestionar_demonio <nombre_demonio> <accion>\n");
    } else {
        registrar_historial(args[0]);
        gestionar_demonio(args[1], args[2]); // args[1] es el nombre del demonio, args[2] es la acción
    }
    continue;
}

// Llamada a función para ejecutar comandos genéricos
if (strcmp(args[0], "ejecutar") == 0) {
    if (args[1] == NULL) {
        printf("Uso: ejecutar <comando>\n");
    } else {
        registrar_historial(args[0]);
        ejecutar_comando(args[1]); // args[1] es el comando a ejecutar
    }
    continue;
}

// Llamada a función para registrar inicio o cierre de sesión
if (strcmp(args[0], "sesion") == 0) {
    if (args[1] == NULL || args[2] == NULL) {
        printf("Uso: sesion <usuario> <accion>\n");
    } else {
        registrar_historial(args[0]);
        registrar_sesion(args[1], args[2]); // args[1] es el usuario, args[2] es la acción (iniciar o cerrar)
    }
    continue;
}

// Llamada a función para realizar transferencia por SCP o FTP
if (strcmp(args[0], "transferencia") == 0) {
    if (args[1] == NULL || args[2] == NULL || args[3] == NULL) {
        printf("Uso: transferencia <origen> <destino> <metodo>\n");
    } else {
        registrar_historial(args[0]);
        transferencia_archivo(args[1], args[2], args[3]); // args[1]: origen, args[2]: destino, args[3]: método (scp o ftp)
    }
    continue;
}


//llamada a funcion para crear usuario
 
if (strcmp(args[0], "usuario") == 0) {
    registrar_historial(args[0]);
    if (args[1] == NULL || args[2] == NULL || args[3] == NULL) {
        printf("Uso: usuario <nombre> <horario> <ips>\n");
    } else {
        agregar_usuario(args[1], args[2], args[3]);
    }
    continue;
}

//Llamada a funcion ir
if (strcmp(args[0], "ir") == 0) {
    registrar_historial(args[0]);
    if (args[1] == NULL) {
        printf("Uso: ir <ruta>\n");
    } else {
        cambiar_directorio(args[1]);
    }
    continue;
}

//Llamada para vim
if (strcmp(args[0], "vim") == 0) {
    registrar_historial(args[0]);
    if (args[1] == NULL) {
        printf("Uso: vim <nombre_archivo>\n");
    } else {
        // Crear un proceso hijo para ejecutar vim
        pid_t pid = fork();
        if (pid == 0) {
            // Proceso hijo: ejecutar vim
            if (execlp("vim", "vim", args[1], NULL) == -1) {
                registrar_error("Error al ejecutar vim");
                exit(EXIT_FAILURE);
            }
        } else if (pid > 0) {
            // Proceso padre: esperar al hijo
            wait(NULL);
        } else {
            registrar_error("Error al crear proceso");
        }
    }
    continue;
}




	}
}

int main() {
    // Iniciar la shell
    printf("Bienvenido a mi_shell. Escribe 'salir' para terminar.\n");
    // Crear el directorio para los logs de error si no existe
    struct stat st;
    if (stat("/var/log/shell", &st) == -1) {
        mkdir("/var/log/shell", 0755);
    }

    shell_loop();
    return 0;
}

