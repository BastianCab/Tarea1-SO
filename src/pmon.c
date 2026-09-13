#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <time.h>

#include "pmon.h"


//Esta estructura agrupa la informacion de los procesos
typedef struct{
    pid_t pid;
    char comando[256];
    char estado;
    unsigned long utime;
    unsigned long stime;
    long rss_kb;
    double porcentaje_cpu;
} ProcesoInfo;


//Bandera que indica cuando llego SIGALRM
static volatile sig_atomic_t actualizar= 0;
static volatile sig_atomic_t salir_pmon= 0;


//Manejador de SIGALRM, solo avisa que corresponde actualizar
static void manejar_alarma(int signal){
    (void)signal;
    actualizar= 1;
}

//Manejador de Ctrl+C, avisa que pmon debe terminar
static void manejar_sigint(int signal){
    (void)signal;
    salir_pmon= 1;
}

//Lee /proc/PID/stat y guarda los datos necesarios
static int leer_stat(pid_t pid, ProcesoInfo *info){
    char ruta[64];
    char linea[4096];

    snprintf(ruta, sizeof(ruta), "/proc/%d/stat", (int)pid);

    FILE *archivo= fopen(ruta, "r");

    if(archivo== NULL){
        return 1;
    }

    if(fgets(linea, sizeof(linea), archivo)== NULL){
        fclose(archivo);
        return 1;
    }

    fclose(archivo);

    char *inicio= strchr(linea, '(');
    char *fin= strrchr(linea, ')');

    if(inicio== NULL || fin== NULL){
        return 1;
    }

    size_t largo= (size_t)(fin - inicio - 1);

    if(largo>= sizeof(info->comando)){
        largo= sizeof(info->comando) - 1;
    }

    memcpy(info->comando, inicio + 1, largo);
    info->comando[largo]= '\0';

    char *resto= fin + 2;

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


//Lee VmRSS desde /proc/PID/status
static int leer_status(pid_t pid, ProcesoInfo *info){
    char ruta[64];
    char linea[512];

    snprintf(ruta, sizeof(ruta), "/proc/%d/status", (int)pid);

    FILE *archivo= fopen(ruta, "r");

    if(archivo== NULL){
        return 1;
    }

    info->rss_kb= -1;

    while(fgets(linea, sizeof(linea), archivo)!= NULL){
        if(sscanf(linea, "VmRSS: %ld kB", &info->rss_kb)== 1){
            break;
        }
    }

    fclose(archivo);

    if(info->rss_kb== -1){
        info->rss_kb= 0;
    }

    return 0;
}

//Calcula porcentaje aproximado de CPU entre dos mediciones
static double calcular_cpu(ProcesoInfo *anterior, ProcesoInfo *actual, double intervalo){
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

//Compara dos procesos para ordenarlos de mayor a menor porcentaje de CPU
static int comparar_cpu(const void *a, const void *b){
    const ProcesoInfo *proceso_a= (const ProcesoInfo *)a;
    const ProcesoInfo *proceso_b= (const ProcesoInfo *)b;

    if(proceso_a->porcentaje_cpu< proceso_b->porcentaje_cpu){
        return 1;
    }

    if(proceso_a->porcentaje_cpu> proceso_b->porcentaje_cpu){
        return -1;
    }

    return 0;
}

//Calcula el tiempo real transcurrido
static double tiempo_transcurrido(struct timespec inicio, struct timespec fin){
    double segundos= (double)(fin.tv_sec - inicio.tv_sec);

    double nanosegundos=
        (double)(fin.tv_nsec - inicio.tv_nsec) / 1000000000.0;

    return segundos + nanosegundos;
}


//Funcion principal del modulo pmon
int ejecutar_pmon(pid_t *pids, int cantidad, int intervalo){
    if(pids== NULL || cantidad<= 0){
        printf("No hay procesos para monitorear\n");
        return 1;
    }

    if(intervalo<= 0){
        intervalo= 2;
    }

    struct sigaction sa_alarm;
    struct sigaction sa_int;
    struct sigaction anterior_alarm;
    struct sigaction anterior_int;

    memset(&sa_alarm, 0, sizeof(sa_alarm));
    memset(&sa_int, 0, sizeof(sa_int));


    //Configura SIGALRM
    sa_alarm.sa_handler= manejar_alarma;
    sigemptyset(&sa_alarm.sa_mask);
    sa_alarm.sa_flags= SA_RESTART;

    if(sigaction(SIGALRM, &sa_alarm, &anterior_alarm)== -1){
        perror("sigaction SIGALRM");
      return 1;
    }


    //Configura SIGINT para salir de pmon con Ctrl+C
    sa_int.sa_handler= manejar_sigint;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags= 0;

    if(sigaction(SIGINT, &sa_int, &anterior_int)== -1){
        perror("sigaction SIGINT");

        //Se restaura SIGALRM porque ya habia sido modificado
        sigaction(SIGALRM, &anterior_alarm, NULL);

        return 1;
    }

    //Bloquea SIGALRM y SIGINT
    sigset_t mascara_bloqueada;
    sigset_t mascara_anterior;
    sigset_t mascara_espera;

    sigemptyset(&mascara_bloqueada);
    sigaddset(&mascara_bloqueada, SIGALRM);
    sigaddset(&mascara_bloqueada, SIGINT);

    if(sigprocmask(SIG_BLOCK, &mascara_bloqueada, &mascara_anterior)== -1){
       perror("sigprocmask");

       sigaction(SIGALRM, &anterior_alarm, NULL);
       sigaction(SIGINT, &anterior_int, NULL);

        return 1;
}

mascara_espera= mascara_anterior;

sigdelset(&mascara_espera, SIGALRM);
sigdelset(&mascara_espera, SIGINT);

    

    ProcesoInfo *anteriores= calloc(cantidad, sizeof(ProcesoInfo));
    ProcesoInfo *actuales= calloc(cantidad, sizeof(ProcesoInfo));
    int *activos= calloc(cantidad, sizeof(int));

    if(anteriores== NULL || actuales== NULL || activos== NULL){
    fprintf(stderr, "Error al reservar memoria\n");

    free(anteriores);
    free(actuales);
    free(activos);

    sigaction(SIGALRM, &anterior_alarm, NULL);
    sigaction(SIGINT, &anterior_int, NULL);
    sigprocmask(SIG_SETMASK, &mascara_anterior, NULL);

    return 1;
}

    int cantidad_activos= 0;

    //Primera medicion de todos los procesos
    for(int i= 0; i< cantidad; i++){
        pid_t pid= pids[i];

        anteriores[i].pid= pid;
        actuales[i].pid= pid;

        if(leer_stat(pid, &anteriores[i])== 1){
            activos[i]= 0;
            continue;
        }

        activos[i]= 1;
        cantidad_activos++;
    }

    if(cantidad_activos== 0){
    printf("No hay procesos validos para monitorear\n");

    free(anteriores);
    free(actuales);
    free(activos);

    sigaction(SIGALRM, &anterior_alarm, NULL);
    sigaction(SIGINT, &anterior_int, NULL);
    sigprocmask(SIG_SETMASK, &mascara_anterior, NULL);

    return 1;
}

    struct timespec tiempo_anterior;
    struct timespec tiempo_actual;

    if(clock_gettime(CLOCK_MONOTONIC, &tiempo_anterior)== -1){
    perror("clock_gettime");

    free(anteriores);
    free(actuales);
    free(activos);

    sigaction(SIGALRM, &anterior_alarm, NULL);
    sigaction(SIGINT, &anterior_int, NULL);
    sigprocmask(SIG_SETMASK, &mascara_anterior, NULL);

    return 1;
}

    printf("Monitoreando %d proceso(s) cada %d segundos...\n",
           cantidad_activos, intervalo);

    actualizar= 0;
    salir_pmon= 0;

    alarm(intervalo);

    while(salir_pmon== 0){

        while(actualizar== 0 && salir_pmon== 0){
            sigsuspend(&mascara_espera);
    }

        if(salir_pmon== 1){
            break;
        }

        if(actualizar== 1){
            actualizar= 0;

        if(clock_gettime(CLOCK_MONOTONIC, &tiempo_actual)== -1){
            perror("clock_gettime");
            alarm(0);

            free(anteriores);
            free(actuales);
            free(activos);


            sigaction(SIGALRM, &anterior_alarm, NULL);
            sigaction(SIGINT, &anterior_int, NULL);
            sigprocmask(SIG_SETMASK, &mascara_anterior, NULL);

            return 1;
}

            double intervalo_real=
                tiempo_transcurrido(tiempo_anterior, tiempo_actual);

            int procesos_restantes= 0;
            ProcesoInfo tabla[cantidad];

            for(int i= 0; i< cantidad; i++){
                if(activos[i]== 0){
                    continue;
                }

                pid_t pid= anteriores[i].pid;
                actuales[i].pid= pid;

                if(leer_stat(pid, &actuales[i])== 1){
                    activos[i]= 0;
                    continue;
                }

                if(leer_status(pid, &actuales[i])== 1){
                    activos[i]= 0;
                    continue;
                }

                actuales[i].porcentaje_cpu=
                    calcular_cpu(&anteriores[i], &actuales[i], intervalo_real);

                //Guarda el proceso en la tabla temporal para luego ordenarlo
                tabla[procesos_restantes]= actuales[i];

                //La medicion actual pasa a ser la anterior
                anteriores[i]= actuales[i];

                procesos_restantes++;
            }

            //Ordena la tabla de mayor a menor porcentaje de CPU
            qsort(tabla, procesos_restantes, sizeof(ProcesoInfo), comparar_cpu);


            //Muestra los procesos ya ordenados
            printf("\n");
            printf("   %-8s %-20s %-10s %-12s %-10s\n",
                   "PID", "COMANDO", "ESTADO", "%CPU", "RSS(KB)");

            for(int i= 0; i< procesos_restantes; i++){

               //El primer proceso es el que tiene mayor uso de CPU
                if(i== 0){
                    printf(">> ");
                }
                else{
                    printf("   ");
                }

            printf("%-8d %-20s %-10c %-12.1f %-10ld\n",
                (int)tabla[i].pid,
                tabla[i].comando,
                tabla[i].estado,
                tabla[i].porcentaje_cpu,
                tabla[i].rss_kb);
}
            tiempo_anterior= tiempo_actual;

            if(procesos_restantes== 0){
                printf("Todos los procesos monitoreados terminaron\n");
                break;
            }

            alarm(intervalo);
        }
    }

    
    alarm(0);

    //Se restauran los manejadores anteriores
    sigaction(SIGALRM, &anterior_alarm, NULL);
    sigaction(SIGINT, &anterior_int, NULL);

    //Se restaura la mascara de señales anterior
    sigprocmask(SIG_SETMASK, &mascara_anterior, NULL);

    free(anteriores);
    free(actuales);
    free(activos);

    if(salir_pmon== 1){
        printf("\nSaliendo de pmon...\n");
    }

    return 0;
}