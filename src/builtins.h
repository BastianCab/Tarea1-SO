#ifndef BUILTINS_H
#define BUILTINS_H

/* Devuelve 1 si args[0] es un comando interno (built-in) y ya fue
 * manejado por completo (la shell no debe hacer fork/exec para él).
 * Devuelve 0 si NO es un built-in (el llamador debe tratarlo como
 * comando externo).
 *
 * 'salir' se pone en 1 si el built-in ejecutado fue "exit"; en ese
 * caso 'codigo_salida' queda con el código que debe usar main() al
 * retornar. */
int builtin_ejecutar(char **args, int *salir, int *codigo_salida);

#endif /* BUILTINS_H */