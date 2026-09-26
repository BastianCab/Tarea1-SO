/*
 * builtins.c — Comandos internos de la shell: cd, exit, jobs, pmon.
 *
 * Estos se ejecutan DIRECTAMENTE por el proceso de la shell (sin
 * fork/exec), porque algunos de ellos (como cd) necesitan modificar
 * el estado del proceso padre y no tendría sentido hacerlo en un
 * hijo que muere inmediatamente después.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "builtins.h"
#include "jobs.h"

int builtin_ejecutar(char **args, int *salir, int *codigo_salida) {
    *salir = 0;

    if (strcmp("exit", args[0]) == 0) {
        *salir = 1;
        *codigo_salida = (args[1] != NULL) ? atoi(args[1]) : 0;
        return 1;
    }

    if (strcmp("cd", args[0]) == 0) {
        if (args[1] != NULL) {
            if (chdir(args[1]) != 0) {
                perror("cd");
            }
        } else {
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
        /* TODO: implementar pmon (Sección 3 del enunciado) */
        printf("pmon: aún no implementado\n");
        return 1;
    }

    return 0; /* no es un built-in */
}