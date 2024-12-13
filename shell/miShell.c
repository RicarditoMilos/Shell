#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h> 
#include <unistd.h> //da funciones que interactuan con el lfs como el fork() para crear nuevos procesos
#include <dirent.h> //da la funcion de listar y tocar directorios, como: opendir() para abrir un directorio y readdir() para leer los nombres de los archivos en un directorio
#include <sys/types.h> //ayuda a definir tipos de datos del sistema, como pid_t() para identificar procesos y mode_t() para permisos de archivos
#include <sys/stat.h> //chmod 
#include <sys/wait.h>// trae las funciones para esperar a los procesos hijos, funciones como wait() y waitpid()
#include <fcntl.h> // Para operaciones con archivos
#include <pwd.h> //para funciones que accedan a informacion del usuario, en este caso, las contrasegnas
#include <grp.h> //lo mismo que pwd pero con grupos
#include <errno.h> //esta cabecera da mensajes de error al usuario 
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
    time_t now = time(NULL); // se obtiene el tiempo actual
    struct tm *t = localtime(&now); //struct tm es una estructura que representa la fecha y la hora desglosada en sus componentes individuales
    if (t != NULL) {
        strftime(buffer, size, "%Y-%m-%d %H:%M:%S", t);         //formatea la fecha y hora en el buffer
    } else { //mensaje de error
        snprintf(buffer, size, "Timestamp no disponible");
    }
}

// Funcion para registrar en el historial
void registrar_historial(const char *comando) {
    const char *log_path = "/home/historial.log"; // Ruta fija para el archivo de log
    FILE *archivo = fopen(log_path, "a");
    if (archivo == NULL) {
        perror("Error al abrir /home/historial.log");
        return;
    }

    char timestamp[64];
    obtener_timestamp(timestamp, sizeof(timestamp));

    fprintf(archivo, "%s: %s\n", timestamp, comando);
    fclose(archivo);
}


// Funcion para registrar errores
void registrar_error(const char *mensaje) {
    FILE *archivo = fopen(ERROR_LOG_FILE, "a"); //se crea puntero que apunta al archivo del error_log_file
    if (archivo == NULL) {
        perror("Error al abrir sistema_error.log");
        return;
    }

    char timestamp[64];
    obtener_timestamp(timestamp, sizeof(timestamp));
    perror(mensaje);
    fprintf(archivo, "%s: ERROR: %s\n", timestamp, mensaje);  //se le informa al usuario
    fclose(archivo);  
}

// Función para dividir el comando en argumentos
void parse_command(char *input, char **args) {
    char *token;
    int i = 0;

    // Dividir el input por espacios (gracias strtok tqm)
    token = strtok(input, " \n"); 
    while (token != NULL) {
        args[i++] = token;
        token = strtok(NULL, " \n");
    }
    args[i] = NULL; // Terminar la lista de argumentos
}

//------------------------------------------------------------------------------//
void copiar_archivo(const char *origen, const char *destino) {
    int src_fd, dest_fd;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read, bytes_written;

    // se abre el archivo de origen en solo lectura
    src_fd = open(origen, O_RDONLY);
    if (src_fd < 0) {
        registrar_error("Error al abrir el archivo de origen");
        return;
    }

    // se abre el archivo de destino en modo escritura, crea el archivo si no existe, y trunca el archivo si ya existe
    dest_fd = open(destino, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dest_fd < 0) {
        registrar_error("Error al crear el archivo de destino");
        close(src_fd);
        return;
    }

    // se lee del archivo origen y se escribe en el archivo de destino en bloques de tamaño BUFFER_SIZE
    while ((bytes_read = read(src_fd, buffer, BUFFER_SIZE)) > 0) {
        bytes_written = write(dest_fd, buffer, bytes_read);
        if (bytes_written != bytes_read) {
            registrar_error("Error al escribir en el archivo de destino");
            close(src_fd);
            close(dest_fd);
            return;
        }
    }

    // Verifica si hubo un error al leer el archivo de origen
    if (bytes_read < 0) {
        registrar_error("Error al leer el archivo de origen");
    }

    // Cierra los archivos de origen y destino
    close(src_fd);
    close(dest_fd);

    // Imprime un mensaje indicando que el archivo ha sido copiado
    printf("Archivo copiado de '%s' a '%s'.\n", origen, destino);
}


// Función para copiar un directorio de forma recursiva
void copiar_directorio(const char *origen, const char *destino) {
    DIR *dir;
    struct dirent *entry;
    struct stat statbuf;
    char src_path[PATH_MAX];
    char dest_path[PATH_MAX];

    // Crea el directorio de destino si no existe, tira error si no se pudo
    if (mkdir(destino, 0755) < 0 && errno != EEXIST) {
        registrar_error("Error al crear el directorio de destino");
        return;
    }

    // Abre el directorio de origen
    dir = opendir(origen);
    if (dir == NULL) {
        registrar_error("Error al abrir el directorio de origen");
        return;
    }

    // se lee las entradas del directorio de origen
    while ((entry = readdir(dir)) != NULL) {
        // Ignora las entradas "." y ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Construye las rutas completas de origen y destino
        snprintf(src_path, sizeof(src_path), "%s/%s", origen, entry->d_name);
        snprintf(dest_path, sizeof(dest_path), "%s/%s", destino, entry->d_name);

        // Obtiene información sobre la entrada actual
        if (stat(src_path, &statbuf) == 0) {
            if (S_ISDIR(statbuf.st_mode)) {
                // Si es un directorio, copiar recursivamente
                copiar_directorio(src_path, dest_path);
            } else if (S_ISREG(statbuf.st_mode)) {
                // Si es un archivo, copiar
                copiar_archivo(src_path, dest_path);
            }
        }
    }

    // Cierra el directorio de origen
    closedir(dir);

    // Imprime un mensaje indicando que el directorio ha sido copiado
    printf("Directorio copiado de '%s' a '%s'.\n", origen, destino);
}


// Función para decidir si se copia un archvo o un directorio
void copiar(const char *origen, const char *destino) {
    struct stat statbuf;

    // Obtiene información sobre el origen
    if (stat(origen, &statbuf) < 0) {
        registrar_error("Error al obtener información del origen");
        return;
    }

    // Verifica si el origen es un directorio
    if (S_ISDIR(statbuf.st_mode)) {
        // Es un directorio: copiar recursivamente
        copiar_directorio(origen, destino);
    } else if (S_ISREG(statbuf.st_mode)) {
        // Es un archivo: copiar
        copiar_archivo(origen, destino);
    } else {
        // El origen no es un archivo ni un directorio válido
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
        // Construir la nueva ruta de destino
        snprintf(nuevo_destino, sizeof(nuevo_destino), "%s/%s", destino, strrchr(origen, '/') ? strrchr(origen, '/') + 1 : origen);

        // Mover el archivo o directorio al nuevo destino
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
        // Verificar el tipo de error
        if (errno == EEXIST) {
            printf("Error: El directorio '%s' ya existe.\n", nombre_directorio);
        } else {
            perror("Error al crear el directorio");
        }
    }
}

//-----------------------------------------------------------------------------------//
//Funcion pwd
void mostrar_directorio_actual() {
    char directorio_actual[PATH_MAX];

    // Obtener el directorio actual
    if (getcwd(directorio_actual, sizeof(directorio_actual)) != NULL) {
        printf("Directorio actual: %s\n", directorio_actual);
    } else {
        perror("Error al obtener el directorio actual");
    }
}
//----------------------------------------------------------------------------------//
//Funcion para cambiar de directorio, cd
void ir(const char *ruta) {
    // Intentar cambiar el dire ctorio
    if (chdir(ruta) == 0) { //devuelve cero si en efecto, se cambió
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
        printf("Error al cambiar propietario del archivo '%s': %s\n", archivo, strerror(errno));
    }
}

//----------------------------------------------------------------------------------//
//Funcion para cambiar contrasena, solo disponible con usuarios con permisos de root
void cambiar_contrasena() {
    const char *usuario = getenv("USER"); // Obtener el nombre del usuario actual
    if (usuario == NULL) {
        printf("No se pudo determinar el usuario actual.\n");
        return;
    }

    printf("Cambiando contrasena para el usuario '%s'.\n", usuario);

    // Crear un comando para llamar a 'passwd' con el usuario actual
    char comando[256];
    snprintf(comando, sizeof(comando), "passwd %s", usuario); //se mete dentro de la cadena comando "passwd 'juan'" por ejemplo

    // Ejecutar el comando
    int resultado = system(comando); //se llama la funcion de cambiar contrasena del sistema
    if (resultado == 0) {
        printf("Contrasena cambiada exitosamente.\n");
    } else {
        printf("Error al cambiar la contrasena.\n");
    }
}
//----------------------------------------------------------------------------------//
//Funcion para agregar usuario con su ip y horario laboral, solo disponible para usuarios con permisos de root
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
// Funciónes para levantar y apagar demonios
void levantar_demonio(const char *nombre_demonio, const char *ruta_binario) {
    printf("Intentando iniciar el demonio '%s'\n", nombre_demonio);

    pid_t pid = fork();
    if (pid == 0) {
        // Proceso hijo se convierte en demonio, tira error si no se puede
        if (setsid() == -1) {
            perror("Error al crear sesión de demonio");
            exit(EXIT_FAILURE);
        }

        // Redirigir entradas/salidas a /dev/null
        freopen("/dev/null", "r", stdin);
        freopen("/dev/null", "w", stdout);
        freopen("/dev/null", "w", stderr);

        // Ejecutar el binario del demonio
        execl(ruta_binario, ruta_binario, (char *)NULL);
        perror("Error al ejecutar el demonio");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        // Proceso padre guarda el PID del demonio
        char pid_path[128];
        snprintf(pid_path, sizeof(pid_path), "/var/run/%s.pid", nombre_demonio);
        FILE *pid_file = fopen(pid_path, "w");
        if (pid_file == NULL) {
            perror("Error al guardar el archivo PID");
            return;
        }
        fprintf(pid_file, "%d\n", pid);
        fclose(pid_file);
        printf("Demonio '%s' iniciado con PID %d\n", nombre_demonio, pid);
    } else {
        perror("Error al crear proceso para el demonio");
    }
}

//como dice la funcion, con esta se apagan demonios que estan encendidos
void apagar_demonio(const char *nombre_demonio) {
    printf("Intentando detener el demonio '%s'\n", nombre_demonio);

    char pid_path[128];
    snprintf(pid_path, sizeof(pid_path), "/var/run/%s.pid", nombre_demonio);

    // Abrir el archivo PID para leer el PID del demonio
    FILE *pid_file = fopen(pid_path, "r");
    if (pid_file == NULL) {
        perror("Error al leer el archivo PID");
        return;
    }

    pid_t pid;
    // Leer el PID del archivo
    if (fscanf(pid_file, "%d", &pid) != 1) {
        perror("Error al leer PID del archivo");
        fclose(pid_file);
        return;
    }
    fclose(pid_file);

    // Enviar la señal SIGTERM al proceso del demonio
    if (kill(pid, SIGTERM) == 0) {
        printf("Demonio '%s' detenido con éxito\n", nombre_demonio);
        remove(pid_path); // Eliminar el archivo PID
    } else {
        perror("Error al detener el demonio");
    }
}




//------------------------------------------------------------------------------//
// Función para ejecutar comandos genéricos
void ejecutar_comando(const char *comando) {
    pid_t pid = fork();
    if (pid == 0) {
        // Proceso hijo para ejecutar el comando
        char *argv[] = {"/bin/sh", "-c", (char *)comando, NULL};

        // Redirigir stdin, stdout y stderr a /dev/null si es necesario
        freopen("/dev/null", "r", stdin);  // Redirigir entrada estándar
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
//Funcion de transferencia de archivo con SCP o FTP
void transferencia_archivo(const char *origen, const char *destino, const char *metodo) {
    char log_path[PATH_MAX] = "Shell_transferencias.log";
    FILE *archivo = fopen(log_path, "a");
    if (archivo == NULL) {
        registrar_error("Error al abrir Shell_transferencias.log");
        return;
    }

    char timestamp[64];
    obtener_timestamp(timestamp, sizeof(timestamp));

    // Verificar si el método es soportado (SCP o FTP)
    if (strcmp(metodo, "scp") == 0 || strcmp(metodo, "ftp") == 0) {
        // Registrar la transferencia en el archivo de log
        fprintf(archivo, "%s: Transferencia iniciada de '%s' a '%s' usando %s\n", timestamp, origen, destino, metodo);
        fclose(archivo);
        // Ejecutar el comando de transferencia
        ejecutar_comando(metodo);  // Ejemplo para delegar en herramientas como SCP
    } else {
        // Registrar el error en el archivo de log si el método no es soportado
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
        printf("Uso: propietario <usuario> <grupo> <archivo(s)>\n");
    } else {
        registrar_historial(args[0]);
        for (int i = 3; args[i] != NULL; i++) { // Iterar sobre los archivos
            propietario(args[1], args[2], args[i]);
        }
    }
    continue;
}


// Llamada a funcion para cambiar la contrasena
if (strcmp(args[0], "contraseña") == 0) {
    registrar_historial(args[0]);
    cambiar_contrasena();
    continue;
}
// Llamadas a función para gestionar demonios
if (strcmp(args[0], "levantar_demonio") == 0) {
    if (args[1] == NULL || args[2] == NULL) {
        printf("Uso: levantar_demonio <nombre_demonio> <ruta_binario>\n");
    } else {
        registrar_historial(args[0]);
        levantar_demonio(args[1], args[2]);
    }
    continue;
}

if (strcmp(args[0], "apagar_demonio") == 0) {
    if (args[1] == NULL) {
        printf("Uso: apagar_demonio <nombre_demonio>\n");
    } else {
        registrar_historial(args[0]);
        apagar_demonio(args[1]);
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

//llamada al pwd
if (strcmp(args[0], "pwd") == 0) {
    mostrar_directorio_actual();
    continue;
}



	}
}

int main() {
    // Obtener el nombre del usuario actual
    const char *usuario = getenv("USER");
    if (usuario == NULL) {
        usuario = "desconocido"; // Nombre genérico si no se puede obtener el usuario
    }

    // Registrar inicio de sesión
    registrar_sesion(usuario, "iniciar");

    // Iniciar la shell personalizada
    printf("Bienvenido a mi_shell. Escribe 'salir' para terminar.\n");

    // Crear el directorio para los logs de error si no existe
    struct stat st;
    if (stat("/var/log/shell", &st) == -1) {
        mkdir("/var/log/shell", 0755);
    }

    // Ejecutar el loop principal de la shell
    shell_loop();

    // Registrar cierre de sesión
    registrar_sesion(usuario, "cerrar");

    return 0;
}
