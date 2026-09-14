#include <stdio.h> //biblioteca de entrada/salida vista en clases. printf(), fprintf(), fopen(), fgets() 
#include <stdlib.h> //Trae otras funciones como atoi() que transforma el pid en numero utilizable
#include <string.h> //Este es para trabajar con strings
#include <unistd.h> //La biblioteca mas importante ya que contiene funciones como fork(), exec(), alarm(), sysconf()
#include <sys/types.h> //este define tipos del sistema, como pid_t 
#include <signal.h> // Manejo de señales como SIGALRM
#include <time.h>   // Medición del tiempo real transcurrido

//Esta estructura agrupa la informacion de los procesos

typedef struct {
    pid_t pid;
    char comando[256];
    char estado;
    unsigned long utime;
    unsigned long stime;
    long rss_kb;
    double porcentaje_cpu;
} ProcesoInfo;

// la bandera para cuando se actualice si es que llego la señal

volatile sig_atomic_t actualizar= 0;

// Manejador de SIGALRM, solo avisa que corresponde actualizar
void manejar_alarma(int signal){
    (void)signal;
    actualizar= 1;
}

//Esto se encarga de leer el /proc/PID/stat y guarda los datos
int leer_stat(pid_t pid, ProcesoInfo *info){
    char ruta[64];
    char linea[4096];

    snprintf(ruta, sizeof(ruta), "/proc/%d/stat", (int)pid);

    FILE *archivo= fopen(ruta, "r");

	//En el caso que haya un error con el archivo ya sea porque no existe o no se pudo abrir lanzara un error

    if(archivo== NULL){
        return 1;
    }
	//En el caso que no se lee la linea completa del archivo

    if(fgets(linea, sizeof(linea), archivo)== NULL){
        fclose(archivo);
        return 1;
    }

    fclose(archivo);


    //  En esta parte busca la parte inicial y final del nombre del proceso en /proc/PID/stat
    char *inicio= strchr(linea, '(');
    char *fin= strrchr(linea, ')');

    if(inicio== NULL || fin== NULL){
        return 1;
    }
	//Calcula el largo del nombre del proceso

    size_t largo= (size_t)(fin - inicio - 1);

    if(largo>= sizeof(info->comando)){
        largo= sizeof(info->comando) - 1;
    }

	//En esta parte se copia el nombre del proceso a la estructura

    memcpy(info->comando, inicio + 1, largo);
    info->comando[largo]= '\0';


    /*
     * Después del nombre vienen:
     *
     * campo 3= estado
     * campo 4= ppid
     * campo 5= pgrp
     * campo 6= session
     * campo 7= tty_nr
     * campo 8= tpgid
     * campo 9= flags
     * campo 10= minflt
     * campo 11= cminflt
     * campo 12= majflt
     * campo 13= cmajflt
     * campo 14= utime
     * campo 15= stime
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

    int leidos= sscanf(
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


    if(leidos!= 13){
        return 1;
    }

    return 0;
}


// Lee VmRSS desde /proc/PID/status 
int leer_status(pid_t pid, ProcesoInfo *info){
    char ruta[64];
    char linea[512];

    snprintf(ruta, sizeof(ruta), "/proc/%d/status", (int)pid);

    FILE *archivo = fopen(ruta, "r");

    if(archivo== NULL){
        return 1;
    }

    info->rss_kb= -1;

	//Aqui se recorre el archivo status linea por linea hasta que se encuentre VmRSS

    while (fgets(linea, sizeof(linea), archivo) != NULL) {

	//en el caso que lo encuentre guarda la memoria residente en kb

        if(sscanf(linea, "VmRSS: %ld kB", &info->rss_kb)== 1){
            break;
        }
    }

    fclose(archivo);

    if(info->rss_kb== -1){
        return 1;
    }

    return 0;
}

// Calcula el porcentaje aproximado de CPU entre dos mediciones

double calcular_cpu(ProcesoInfo *anterior, ProcesoInfo *actual, double intervalo){
    unsigned long cpu_anterior= anterior->utime + anterior->stime;
    unsigned long cpu_actual= actual->utime + actual->stime;
    unsigned long diferencia_ticks= cpu_actual - cpu_anterior;
    long ticks_por_segundo= sysconf(_SC_CLK_TCK);

    if(ticks_por_segundo<= 0 || intervalo<= 0){
        return 0.0;
    }
    double tiempo_cpu= (double)diferencia_ticks / ticks_por_segundo;
    double porcentaje= (tiempo_cpu / intervalo) * 100.0;
    return porcentaje;
}

double tiempo_transcurrido(struct timespec inicio, struct timespec fin){
    double segundos= (double)(fin.tv_sec - inicio.tv_sec);
    double nanosegundos= (double)(fin.tv_nsec - inicio.tv_nsec) / 1000000000.0;

    return segundos + nanosegundos;
}

// Aqui cada letra tiene su estado incorporado 
const char *nombre_estado(char estado){
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


int main(int argc, char *argv[]){
    if(argc< 2){
        printf("Uso: %s PID1 [PID2 PID3 ...]\n", argv[0]);
        return 1;
    }

    //Esto le dice al sistema que ejecute manejar_alarma() cuando reciba SIGALRM
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));

    sa.sa_handler= manejar_alarma;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags= SA_RESTART;

    if(sigaction(SIGALRM, &sa, NULL)== -1){
        perror("sigaction");
        return 1;
    }

    //Cantidad de procesos recibidos
    int cantidad= argc - 1;

    //Se reservan estructuras para las mediciones anterior y actual
    ProcesoInfo *anteriores= calloc(cantidad, sizeof(ProcesoInfo));
    ProcesoInfo *actuales= calloc(cantidad, sizeof(ProcesoInfo));

    //Indica si cada proceso sigue activo
    int *activos= calloc(cantidad, sizeof(int));

    if(anteriores== NULL || actuales== NULL || activos== NULL){
        fprintf(stderr, "Error al reservar memoria\n");

        free(anteriores);
        free(actuales);
        free(activos);

        return 1;
    }

    int cantidad_activos= 0;

    //Primera medicion de todos los procesos
    for(int i= 0; i< cantidad; i++){
        pid_t pid= (pid_t)atoi(argv[i + 1]);

        anteriores[i].pid= pid;
        actuales[i].pid= pid;

        if(leer_stat(pid, &anteriores[i])== 1){
            fprintf(stderr, "No se pudo leer el proceso %d\n", (int)pid);
            activos[i]= 0;
            continue;
        }

        activos[i]= 1;
        cantidad_activos++;
    }

    //Si ninguno de los PID entregados existe, se termina el programa
    if(cantidad_activos== 0){
        printf("No hay procesos validos para monitorear\n");

        free(anteriores);
        free(actuales);
        free(activos);

        return 1;
    }

    int intervalo= 2;

    struct timespec tiempo_anterior;
    struct timespec tiempo_actual;

    //Guarda el instante de la primera medicion
    if(clock_gettime(CLOCK_MONOTONIC, &tiempo_anterior)== -1){
        perror("clock_gettime");

        free(anteriores);
        free(actuales);
        free(activos);

        return 1;
    }

    printf("Monitoreando %d proceso(s) cada %d segundos...\n",
           cantidad_activos, intervalo);

    //Programa la primera alarma
    alarm(intervalo);

    while(1){

        //Espera hasta recibir la señal
        pause();

        if(actualizar== 1){
            actualizar= 0;

            if(clock_gettime(CLOCK_MONOTONIC, &tiempo_actual)== -1){
                perror("clock_gettime");
                break;
            }

            double intervalo_real=
                tiempo_transcurrido(tiempo_anterior, tiempo_actual);

            int procesos_restantes= 0;

            printf("\n");
            printf("%-8s %-20s %-10s %-12s %-10s\n",
                   "PID", "COMANDO", "ESTADO", "%CPU", "RSS(KB)");

            for(int i= 0; i< cantidad; i++){

                //Si este proceso ya termino, se ignora
                if(activos[i]== 0){
                    continue;
                }

                pid_t pid= anteriores[i].pid;
                actuales[i].pid= pid;

                //Nueva lectura del proceso
                if(leer_stat(pid, &actuales[i])== 1){
                    activos[i]= 0;
                    continue;
                }

                //Lectura de memoria residente
                if(leer_status(pid, &actuales[i])== 1){
                    activos[i]= 0;
                    continue;
                }

                //Calcula CPU entre la medicion anterior y la actual
                actuales[i].porcentaje_cpu=
                    calcular_cpu(&anteriores[i], &actuales[i], intervalo_real);

                printf("%-8d %-20s %-10c %-12.1f %-10ld\n",
                       (int)actuales[i].pid,
                       actuales[i].comando,
                       actuales[i].estado,
                       actuales[i].porcentaje_cpu,
                       actuales[i].rss_kb);

                //La medicion actual pasa a ser la anterior
                anteriores[i]= actuales[i];

                procesos_restantes++;
            }

            //El tiempo actual pasa a ser el anterior
            tiempo_anterior= tiempo_actual;

            //Si todos los procesos terminaron, se deja de monitorear
            if(procesos_restantes== 0){
                printf("Todos los procesos monitoreados terminaron\n");
                break;
            }

            //Programa el siguiente refresco
            alarm(intervalo);
        }
    }

    //Cancela cualquier alarma pendiente
    alarm(0);

    //Libera la memoria reservada
    free(anteriores);
    free(actuales);
    free(activos);

    return 0;
}