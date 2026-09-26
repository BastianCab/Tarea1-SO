#ifndef EXECUTOR_H
#define EXECUTOR_H

/* Ejecuta un comando externo (ya determinado que NO es un built-in).
 *
 *   args[]            - tokens del comando (puede incluir <, >, >>, |)
 *   count              - cantidad de tokens en args[] (sin el NULL final)
 *   background         - 1 si la línea terminaba en '&', 0 si no
 *   cmdline_original    - texto tal cual lo escribió el usuario (para
 *                         mostrarlo en "jobs" y en el aviso "Done")
 *
 * Maneja fork(), separación de grupo de proceso + señales en el
 * hijo (R6), extracción de redirecciones (R3) y pipes (R4), y
 * registro en la tabla de jobs si es background (R5), o
 * waitpid() bloqueante si es foreground. */
void executor_ejecutar(char **args, int count, int background,
                        const char *cmdline_original);

#endif /* EXECUTOR_H */