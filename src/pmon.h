#ifndef PMON_H
#define PMON_H

#include <sys/types.h>

//Ejecuta pmon sobre una lista de procesos
int ejecutar_pmon(pid_t *pids, int cantidad, int intervalo);

#endif