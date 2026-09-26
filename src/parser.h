#ifndef PARSER_H
#define PARSER_H

#define MAX_ARGS 100

/* Tokeniza 'input' (que SERÁ MODIFICADO por strtok, insertando '\0'
 * en los espacios) en el arreglo 'args', terminado en NULL.
 * Devuelve la cantidad de tokens encontrados (sin contar el NULL). */
int parser_tokenizar(char *input, char **args);

/* Revisa si el ÚLTIMO token de args[] (según 'count') es "&". Si lo
 * es: lo quita del arreglo (lo pone en NULL), decrementa *count, y
 * devuelve 1. Si no lo es, no toca nada y devuelve 0. */
int parser_detectar_background(char **args, int *count);

#endif /* PARSER_H */