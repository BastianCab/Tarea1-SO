/*
 * executor.c — Ejecución de comandos externos: fork, redirección
 * (R3), pipes (R4), background (R5) y señales del hijo (R6).
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#include "executor.h"
#include "jobs.h"
#include "signals.h"
#include "redirections.h"
#include "pipes.h"

void executor_ejecutar(char **args, int count, int background,
                        const char *cmdline_original) {
    pid_t pid = fork();

    if (pid == 0) {
        // R6 fix: si el comando es background, separarlo en su PROPIO
        // grupo de proceso ANTES de restaurar las señales. Sin esto,
        // Ctrl+C mata también a los procesos en background, porque el
        // driver TTY manda SIGINT a TODO el grupo de proceso que la
        // terminal tiene marcado como "foreground". Esto también
        // cubre pipelines en background ("cmd1 | cmd2 &"), porque los
        // sub-procesos que forkea creaPipe() más abajo HEREDAN este
        // mismo pgid nuevo (fork() copia el pgid del padre en ese
        // instante).
        child_new_process_group_if_background(background);

        // Restaurar señales por defecto para que Ctrl+C sí pueda
        // matar al comando en foreground.
        child_signals_restore_default();

        // Extraer redirecciones de entrada (<) y salida (>, >>)
        char *archivo_in = NULL;
        char *archivo_out = NULL;
        int append = 0;

        for (int j = 0; j < count; j++) {
            if (args[j] != NULL) {
                if (strcmp(args[j], "<") == 0) {
                    if (j + 1 < count) archivo_in = args[j + 1];
                    args[j] = NULL;
                } else if (strcmp(args[j], ">") == 0) {
                    if (j + 1 < count) archivo_out = args[j + 1];
                    append = 0;
                    args[j] = NULL;
                } else if (strcmp(args[j], ">>") == 0) {
                    if (j + 1 < count) archivo_out = args[j + 1];
                    append = 1;
                    args[j] = NULL;
                }
            }
        }

        redireccionar(archivo_in, archivo_out, append);

        // Contar tuberías (|) para decidir el flujo de ejecución
        int num_pipes = 0;
        for (int j = 0; j < count; j++) {
            if (args[j] != NULL && strcmp(args[j], "|") == 0) {
                num_pipes++;
            }
        }

        if (num_pipes > 0) {
            int cmdsTotal = num_pipes + 1;
            char ***cmds = malloc(cmdsTotal * sizeof(char **));
            int cmd_idx = 0;

            cmds[0] = &args[0];

            for (int j = 0; j < count; j++) {
                if (args[j] != NULL && strcmp(args[j], "|") == 0) {
                    args[j] = NULL;
                    cmds[++cmd_idx] = &args[j + 1];
                }
            }

            creaPipe(cmds, cmdsTotal);
            free(cmds);
            exit(0);

        } else {
            if (execvp(args[0], args) == -1) {
                perror("Error al ejecutar comando");
                exit(1);
            }
        }

    } else if (pid > 0) {
        if (background) {
            jobs_add(pid, cmdline_original);
        } else {
            waitpid(pid, NULL, 0);
        }
    } else {
        perror("Error al crear proceso hijo");
    }
}