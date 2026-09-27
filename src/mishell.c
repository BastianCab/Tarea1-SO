/*
 * mishell.c — Ciclo principal de la shell.
 *
 * Solo orquesta: lee una línea, la tokeniza (parser.c), revisa si es
 * un built-in (builtins.c), y si no lo es, delega su ejecución
 * (executor.c), que a su vez usa jobs.c (R5), signals.c (R6),
 * redirections.c (R3) y pipes.c (R4).
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "parser.h"
#include "builtins.h"
#include "executor.h"
#include "signals.h"
#include "jobs.h"
#include "history.h"

#define MAXIM 1024

int main() {
    char input_copy[MAXIM];
    char *args[MAX_ARGS];
    char cwd[1024];
    char prompt[1024 + 16];

    shell_signals_init(); // R6: la shell ignora Ctrl+C y Ctrl+Barra invertida
    jobs_init();            // R5: instala el manejador de SIGCHLD
    history_init();         // Bonus: carga el historial previo, si existe

    while (1) {

        jobs_notify_done(); // R5: avisar jobs que terminaron

        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            snprintf(prompt, sizeof(prompt), "MiShell:%s$ ", cwd);
        } else {
            perror("Error al obtener directorio");
            snprintf(prompt, sizeof(prompt), "MiShell:?$ ");
        }

        // readline() imprime el prompt Y maneja toda la edición de
        // línea (flechas ↑/↓ para navegar el historial, Ctrl+A/E,
        // etc.). Devuelve una cadena reservada con malloc(), o NULL
        // en Ctrl+D (EOF) lo mismo que fgets() devolviendo
        // NULL, pero SIN el '\n' final (a diferencia de fgets()).
        char *input = history_leer_linea(prompt);
        if (input == NULL) {
            printf("\n");
            break; // Ctrl+D
        }
        if (strlen(input) == 0) {
            free(input);
            continue;
        }

        // Bonus: registrar la línea en el historial (memoria + disco)
        history_agregar(input);

        // Copia de la línea original ANTES de tokenizar (strtok
        // modifica 'input'), para mostrarla en "jobs"/"Done".
        strncpy(input_copy, input, MAXIM - 1);
        input_copy[MAXIM - 1] = '\0';

        int count = parser_tokenizar(input, args);
        if (count == 0) {
            free(input);
            continue;
        }

        int background = parser_detectar_background(args, &count);
        if (count == 0) {
            free(input);
            continue; // la línea era solo "&"
        }

        int salir = 0;
        if (builtins_ejecutar(args, &salir)) {
            free(input);
            if (salir) {
                break;
            }
            continue;
        }

        executor_ejecutar(args, count, background, input_copy);
        free(input); // readline() reserva memoria; hay que liberarla
    }

    history_guardar(); // Bonus: respaldo final del historial a disco
    return 0;
}