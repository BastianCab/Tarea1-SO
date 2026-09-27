#ifndef BUILTINS_H
#define BUILTINS_H

/* Si args[0] es un comando interno (cd, exit, jobs, pmon), lo
 * ejecuta por completo y devuelve 1 (la shell NO debe hacer
 * fork/exec para él). Si no es un built-in, devuelve 0.
 *
 * Si el built-in ejecutado fue "exit", pone *salir = 1 para que
 * main() sepa que debe terminar el ciclo. */
int builtins_ejecutar(char **args, int *salir);

#endif /* BUILTINS_H */