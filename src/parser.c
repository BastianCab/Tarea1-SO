/*
 * parser.c — Tokenización de la línea de entrada y detección del
 * operador '&' (ejecución en background).
 */

#include <string.h>

#include "parser.h"

int parser_tokenizar(char *input, char **args) {
    int i = 0;
    args[i] = strtok(input, " ");
    // Límite a MAX_ARGS - 1 para siempre dejar espacio al NULL final
    // y evitar desbordar el arreglo 'args' si el usuario escribe una
    // línea con muchísimos tokens.
    while (args[i] != NULL && i < MAX_ARGS - 1) {
        i++;
        args[i] = strtok(NULL, " ");
    }
    args[i] = NULL;
    return i;
}

int parser_detectar_background(char **args, int *count) {
    if (*count <= 0) {
        return 0;
    }
    int ultimo = *count - 1;
    if (strcmp(args[ultimo], "&") == 0) {
        args[ultimo] = NULL;
        (*count)--;
        return 1;
    }
    return 0;
}