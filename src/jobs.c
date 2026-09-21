/*
 * jobs.c — Implementación de R5: ejecución en background con
 *          recolección asíncrona vía SIGCHLD.
 *
 * Detalle de diseño importante:
 *   El manejador de señal NO imprime nada directamente. printf() no es
 *   "async-signal-safe" (puede usar buffers/locks internos que no son
 *   seguros de tocar dentro de un manejador). Por eso el manejador solo
 *   actualiza banderas de estado (marca JOB_DONE y guarda el status);
 *   la impresión real ocurre después, en el ciclo principal, en
 *   jobs_notify_done().
 */

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>
#include <unistd.h>

#include "jobs.h"

job_t jobs[MAX_JOBS];
int   next_job_id = 1;

/*
 * sigchld_handler — manejador de SIGCHLD.
 *
 * Por que un ciclo con WNOHANG y no una sola llamada a waitpid():
 *   Las señales de Unix NO se encolan. Si terminan dos o más hijos
 *   casi al mismo tiempo (por ejemplo dos jobs en background que
 *   acaban juntos), el kernel puede llegar a entregar UNA sola señal
 *   SIGCHLD representando "uno o más hijos cambiaron de estado", no
 *   una señal por cada hijo. Si aquí recogiéramos solo un hijo con una
 *   única llamada, el resto quedaría como zombie para siempre, porque
 *   ya no habrá otra señal pendiente que nos avise de ellos.
 *
 *   La solución estándar es drenar TODOS los hijos terminados
 *   disponibles en este instante, repitiendo waitpid(-1, &status,
 *   WNOHANG) hasta que ya no quede ninguno (devuelve 0) o hasta que
 *   no queden hijos en absoluto (devuelve -1, típicamente con
 *   errno == ECHILD).
 */
static void sigchld_handler(int signo) {
    (void)signo;
    int status;
    pid_t pid;
    int saved_errno = errno; /* waitpid puede modificar errno; lo
                                 restauramos al salir para no romper
                                 una syscall interrumpida en el
                                 código que estaba corriendo cuando
                                 llegó la señal. */

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        for (int i = 0; i < MAX_JOBS; i++) {
            if (jobs[i].in_use && jobs[i].pid == pid &&
                jobs[i].state == JOB_RUNNING) {
                jobs[i].state  = JOB_DONE;
                jobs[i].status = status;
                break;
            }
        }
        /* Nota: si 'pid' no corresponde a ningún job en background
         * (por ejemplo, es un hijo de foreground que terminó y que
         * pronto será recogido también por el waitpid() bloqueante
         * en el flujo de foreground), simplemente lo ignoramos aquí:
         * ya fue recolectado por esta llamada a waitpid(), así que no
         * queda zombie de todas formas. */
    }

    errno = saved_errno;
}

void jobs_init(void) {
    memset(jobs, 0, sizeof(jobs));
    next_job_id = 1;

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    /* SA_RESTART: si una syscall (ej. read() del prompt) es
     *   interrumpida por SIGCHLD, que se reinicie automáticamente en
     *   vez de fallar con EINTR. */
    sa.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &sa, NULL);
}

int jobs_add(pid_t pid, const char *cmdline) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (!jobs[i].in_use) {
            jobs[i].in_use   = 1;
            jobs[i].job_id   = next_job_id++;
            jobs[i].pid      = pid;
            jobs[i].state    = JOB_RUNNING;
            jobs[i].notified = 0;
            jobs[i].status   = 0;
            strncpy(jobs[i].cmdline, cmdline, CMDLINE_MAX - 1);
            jobs[i].cmdline[CMDLINE_MAX - 1] = '\0';

            /* Requisito: mostrar de inmediato "[N] PID" */
            printf("[%d] %d\n", jobs[i].job_id, (int)pid);
            fflush(stdout);

            return jobs[i].job_id;
        }
    }
    fprintf(stderr, "mishell: demasiados jobs en background\n");
    return -1;
}

void jobs_notify_done(void) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].in_use && jobs[i].state == JOB_DONE &&
            !jobs[i].notified) {

            /* Formato pedido: [1]+ Done sleep 30 */
            printf("[%d] + Done %s\n",
                   jobs[i].job_id, jobs[i].cmdline);
            fflush(stdout);

            jobs[i].notified = 1;
            jobs[i].in_use   = 0; /* liberar la ranura: ya se avisó */
        }
    }
}

void jobs_print_list(void) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].in_use) {
            const char *estado =
                (jobs[i].state == JOB_RUNNING) ? "Ejecutando" : "Done";
            printf("[%d] %s %s\n", jobs[i].job_id, estado, jobs[i].cmdline);
        }
    }
}