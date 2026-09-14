/*
 * signals.h - r6: manejo de ctrl+c y ctrl+\
 *
 * que hace esto:
 * 1. hace que la shell ignore el ctrl+c para que no muera de la nada si lo apretan en el prompt
 * 2. le da una funcion al proceso hijo para que la llame despues del fork y antes del execvp
 *    asi los comandos que ejecutas si se pueden detener normal con ctrl+c
 *
 * nota: lo del sigchld (r5) esta en jobs.c/jobs.h porque va amarrado a la tabla de jobs
 * aca solo vemos lo de sigint y sigquit para el r6
 */

#ifndef SIGNALS_H
#define SIGNALS_H

/* hace que la shell principal ignore ctrl+c y ctrl+\
 * llamala una pura vez al principio del main. */
void shell_signals_init(void);

/* le quita la inmunidad heredada al hijo y vuelve a la accion por defecto
 * va en el hijo, justo despues del fork y antes del execvp
 * sirve para que los comandos en primer plano si mueran con ctrl+c */
void child_signals_restore_default(void);

/* si el comando es background, mete al hijo en un grupo de procesos nuevo y aislado
 * asi te aseguras que un ctrl+c en la terminal no lo mate por accidente
 * si no es background, no hace nada y deja que el ctrl+c si lo alcance.
 * va en el hijo, antes del execvp. */
void child_new_process_group_if_background(int background);

#endif /* SIGNALS_H */