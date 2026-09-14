/*
 * jobs.h — Estructuras y funciones para R5 (ejecución en background)
 *
 * Este módulo se integra con el resto de la shell (mishell.c) que ya
 * implementa R1-R4 (fork/exec, redirección, pipes). Aquí solo se agrega
 * la lógica de:
 *   - Registrar jobs en background.
 *   - Recolectarlos de forma asíncrona vía SIGCHLD + waitpid(WNOHANG).
 *   - Reportar "[N]+ Done <cmd>" antes de mostrar el próximo prompt.
 *   - Comando interno "jobs".
 */

#ifndef JOBS_H
#define JOBS_H

#include <sys/types.h>
#include <signal.h>

#define MAX_JOBS 64
#define CMDLINE_MAX 256

typedef enum {
    JOB_RUNNING,
    JOB_DONE
} job_state_t;

typedef struct {
    int         job_id;              /* Número visible: [1], [2], ... */
    pid_t       pid;                 /* PID representativo del job.
                                         En una pipeline, se usa el PID
                                         del ÚLTIMO comando (es el que
                                         define cuándo "termina" el job
                                         desde el punto de vista del
                                         usuario, igual que hace bash). */
    char        cmdline[CMDLINE_MAX];/* Texto original, para "jobs" y "Done" */
    job_state_t state;
    int         status;              /* status devuelto por waitpid, cuando
                                         ya terminó */
    int         notified;            /* 1 si ya se imprimió "Done" */
    int         in_use;              /* 1 si esta ranura está ocupada */
} job_t;

/* Tabla global de jobs. 'extern' aquí, definida una sola vez en jobs.c */
extern job_t jobs[MAX_JOBS];
extern int   next_job_id;

/* Debe llamarse una vez al iniciar la shell, para instalar el
 * manejador de SIGCHLD con sigaction(). */
void jobs_init(void);

/* Registra un nuevo job en background. 'pid' es el PID representativo
 * (último proceso de la pipeline), 'cmdline' es el texto tal cual lo
 * escribió el usuario (sin el '&'), para mostrarlo después.
 * Imprime inmediatamente "[N] PID". Devuelve el job_id asignado, o -1
 * si la tabla está llena. */
int jobs_add(pid_t pid, const char *cmdline);

/* Debe llamarse justo antes de mostrar cada prompt nuevo. Recorre la
 * tabla e imprime "[N]+ Done <cmd>" para los jobs que el manejador de
 * SIGCHLD marcó como terminados y aún no se han notificado. Luego
 * libera esas ranuras de la tabla. */
void jobs_notify_done(void);

/* Implementación del builtin "jobs": lista los jobs en ejecución. */
void jobs_print_list(void);

#endif /* JOBS_H */