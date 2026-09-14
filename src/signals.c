/*
 * signals.c - r6: implementacion de ctrl+c y ctrl+\
 *
 * usamos sigaction() en vez de signal() porque lo pide el enunciado
 * y es mas seguro en posix para usar mascaras y flags
 */

#include <signal.h>
#include <string.h>
#include <unistd.h>

#include "signals.h"

void shell_signals_init(void) {
    struct sigaction sa_ign;
    memset(&sa_ign, 0, sizeof(sa_ign));
    sa_ign.sa_handler = SIG_IGN;
    sigemptyset(&sa_ign.sa_mask);
    
    /* sa_restart: si la shell esta bloqueada esperando que escribas algo
     * y llega un ctrl+c, esto hace que se reinicie sola y no tire error */
    sa_ign.sa_flags = SA_RESTART;

    /* la shell ignora el ctrl+c por completo */
    sigaction(SIGINT, &sa_ign, NULL);
    /* y tambien el ctrl+\ */
    sigaction(SIGQUIT, &sa_ign, NULL);
}

void child_signals_restore_default(void) {
    struct sigaction sa_dfl;
    memset(&sa_dfl, 0, sizeof(sa_dfl));
    sa_dfl.sa_handler = SIG_DFL;
    sigemptyset(&sa_dfl.sa_mask);
    sa_dfl.sa_flags = 0;

    /* el hijo vuelve a la normalidad para que el ctrl+c si lo mate
     * va antes del execvp porque la inmunidad de ignorar senales sobrevive
     * al execvp, asi que hay que quitarsela antes de ejecutar el comando */
    sigaction(SIGINT, &sa_dfl, NULL);
    sigaction(SIGQUIT, &sa_dfl, NULL);
}

void child_new_process_group_if_background(int background) {
    /*
     * el sistema le manda el ctrl+c a todo el grupo de primer plano.
     * si el comando es background, lo aislamos en un grupo nuevo con setpgid(0,0)
     * si no hacemos esto, el proceso hereda el grupo de la shell y muere 
     * igual con el ctrl+c, lo cual rompe el requisito r6
     */
    if (background) {
        setpgid(0, 0);
    }
    /* si es foreground no tocamos nada, se queda en el mismo grupo
     * de la shell para que el ctrl+c si lo alcance */
}