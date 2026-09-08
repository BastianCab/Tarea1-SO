#include <stdio.h> //biblioteca de entrada/salida vista en clases. printf() fprintf() etc...
#include <stdlib.h> //Trae otras funciones como atoi() que transforma el pid en numero utilizable
#include <string.h> //Este es para trabajar con strings
#include <unistd.h> //La biblioteca mas importante ya que contiene funciones como fork(), exec(), etc...
#include <sys/types.h> //Por ultimo , este define tipos del sistema, como pid_t 

//Esta estructura agrupa la informacion de los procesos

typedef struct {
    pid_t pid;
    char comando[256];
    char estado;
    unsigned long utime;
    unsigned long stime;
    long rss_kb;
} ProcesoInfo;


//Esto se encarga de leer el /proc/PID/stat y guarda los datos
int leer_stat(pid_t pid, ProcesoInfo *info)
{
    char ruta[64];
    char linea[4096];

    snprintf(ruta, sizeof(ruta), "/proc/%d/stat", (int)pid);

    FILE *archivo = fopen(ruta, "r");

	//En el caso que haya un error con el archivo ya sea porque no existe o no se pudo abrir lanzara un error

    if (archivo == NULL) {
        return 1;
    }
	//En el caso que no se lle la linea completa del archivo

    if (fgets(linea, sizeof(linea), archivo) == NULL) {
        fclose(archivo);
        return 1;
    }

    fclose(archivo);


    //  En esta parte busca la parte inicial y final del nombre del proceso en /proc/PID/stat
    char *inicio = strchr(linea, '(');
    char *fin = strrchr(linea, ')');

    if (inicio == NULL || fin == NULL) {
        return 1;
    }
	//Calcula el largo del nombre del proceso

    size_t largo = (size_t)(fin - inicio - 1);

    if (largo >= sizeof(info->comando)) {
        largo = sizeof(info->comando) - 1;
    }

	//En esta parte se copia el nombre del proceso a la estructura

    memcpy(info->comando, inicio + 1, largo);
    info->comando[largo] = '\0';


    /*
     * Después del nombre vienen:
     *
     * campo 3  = estado
     * campo 4  = ppid
     * campo 5  = pgrp
     * campo 6  = session
     * campo 7  = tty_nr
     * campo 8  = tpgid
     * campo 9  = flags
     * campo 10 = minflt
     * campo 11 = cminflt
     * campo 12 = majflt
     * campo 13 = cmajflt
     * campo 14 = utime
     * campo 15 = stime
     */

    char *resto = fin + 2;

    int ppid;
    int pgrp;
    int session;
    int tty_nr;
    int tpgid;

    unsigned int flags;

    unsigned long minflt;
    unsigned long cminflt;
    unsigned long majflt;
    unsigned long cmajflt;

	//Aqui lee los campos de /proc/PID/stat en orden
	//los campos intermedios solo se guardan para llegar a estado, utime y stime

    int leidos = sscanf(
        resto,
        "%c %d %d %d %d %d %u %lu %lu %lu %lu %lu %lu",
        &info->estado,
        &ppid,
        &pgrp,
        &session,
        &tty_nr,
        &tpgid,
        &flags,
        &minflt,
        &cminflt,
        &majflt,
        &cmajflt,
        &info->utime,
        &info->stime
    );


    if (leidos != 13) {
        return 1;
    }

    return 0;
}


// Lee VmRSS desde /proc/PID/status 
int leer_status(pid_t pid, ProcesoInfo *info)
{
    char ruta[64];
    char linea[512];

    snprintf(ruta, sizeof(ruta), "/proc/%d/status", (int)pid);

    FILE *archivo = fopen(ruta, "r");

    if (archivo == NULL) {
        return 1;
    }

    info->rss_kb = -1;

	//Aqui se recorre el archivo status linea por linea hasta que se encuentre VmRSS

    while (fgets(linea, sizeof(linea), archivo) != NULL) {

	//en el caso que lo encuentre guarda la memoria residente en kb

        if (sscanf(linea, "VmRSS: %ld kB", &info->rss_kb) == 1) {
            break;
        }
    }

    fclose(archivo);

    return 0;
}


// Aqui cada letra tiene su estado incorporado 
const char *nombre_estado(char estado)
{
    switch (estado) {

        case 'R':
            return "ejecutando";

        case 'S':
            return "durmiendo";

        case 'D':
            return "espera no interrumpible";

        case 'T':
            return "detenido";

        case 'Z':
            return "zombie";

        default:
            return "otro";
    }
}


int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("Uso: %s PID\n", argv[0]);
        return 1;
    }

	//Se pasa el PID a un valor numerico para poder manejarlo con el atoi()

    pid_t pid = (pid_t)atoi(argv[1]);

    ProcesoInfo proceso;

    proceso.pid = pid;


    if (leer_stat(pid, &proceso) == 1) {

        fprintf(
            stderr,
            "No se pudo leer /proc/%d/stat\n",
            (int)pid
        );

        return 1;
    }


    if (leer_status(pid, &proceso) == 1) {

        fprintf(
            stderr,
            "No se pudo leer /proc/%d/status\n",
            (int)pid
        );

        return 1;
    }


    printf("PID:     %d\n", (int)proceso.pid);
    printf("Comando: %s\n", proceso.comando);

    printf(
        "Estado:  %c (%s)\n",
        proceso.estado,
        nombre_estado(proceso.estado)
    );

    printf("utime:   %lu ticks\n", proceso.utime);
    printf("stime:   %lu ticks\n", proceso.stime);
    printf("VmRSS:   %ld KB\n", proceso.rss_kb);


    return 0;
}
