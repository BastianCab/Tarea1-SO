/*
 * builtins.c — Comandos internos: cd, exit, jobs, pmon.
 *
 * Se ejecutan directamente en el proceso de la shell (sin fork),
 * porque "cd" necesita modificar el directorio del proceso PADRE
 * (si se hiciera en un hijo, el chdir() solo afectaría a ese hijo,
 * que muere inmediatamente después, y la shell quedaría intacta).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "builtins.h"
#include "jobs.h"
#include "pmon.h"

int builtins_ejecutar(char **args, int *salir) {
    *salir = 0;

    if (strcmp("exit", args[0]) == 0) {
        *salir = 1;
        return 1;
    }

    if (strcmp("cd", args[0]) == 0) {
        if (args[1] != NULL) {
            if (chdir(args[1]) != 0) {
                perror("cd");
            }
        } else {
            // Sin argumentos, "cd" debe ir a $HOME (comportamiento
            // estándar en bash y demás shells).
            char *home = getenv("HOME");
            if (home == NULL) {
                fprintf(stderr, "cd: no se pudo determinar $HOME\n");
            } else if (chdir(home) != 0) {
                perror("cd");
            }
        }
        return 1;
    }

    if (strcmp("jobs", args[0]) == 0) {
        jobs_print_list();
        return 1;
    }

    if (strcmp("pmon", args[0]) == 0) {
        int intervalo = 2;
        if (args[1] != NULL) {
            intervalo = atoi(args[1]);
        }

        // Extraer los PIDs de los jobs en background actualmente activos
        pid_t pids_activos[MAX_JOBS];
        int cantidad_pids = 0;
        for (int j = 0; j < MAX_JOBS; j++) {
            if (jobs[j].in_use) {
                pids_activos[cantidad_pids++] = jobs[j].pid;
            }
        }

        if (cantidad_pids > 0) {
            ejecutar_pmon(pids_activos, cantidad_pids, intervalo);
        } else {
            printf("No hay procesos en background para monitorear.\n");
        }
        return 1;
    }

    return 0; /* no es un built-in */
}