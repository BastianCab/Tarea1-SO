#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>

//Función para conectar varios comandos por medio de pipes
/* char*** cmds es un puntero hacia una lista que contiene los
   distintos comandos, cada comando con su nombre y flags */

/* cmdsTotal para saber cuantos comandos hay en total para
   conectarlos con pipes */

void creaPipe(char*** cmds, int cmdsTotal) {
    int entradaAc = STDIN_FILENO;
    int p[2];

    //Un arreglo que contendrá los pids para cada proceso
    pid_t *pids = malloc(sizeof(pid_t) * cmdsTotal);
    for (int i = 0; i < cmdsTotal; i++) {
        //Crea una pipe por cada comando, exepto para el final
        if (i < cmdsTotal - 1) {
            if (pipe(p) == -1) {
                perror("Error al crear pipe");
                free(pids);
                return;
            }
        }

        pids[i] = fork();
        //Para el proceso hijo
        if (pids[i] == 0) {

        /*Si no es el primer comando (la entrada actual no se ha modificado)
          conecta la entrada del comando a la pipe anterior*/
            if (entradaAc != STDIN_FILENO) {
                dup2(entradaAc, STDIN_FILENO);
                close(entradaAc);
            }

        /*Si el comando aun no es el final, conecta la salida del comando
          a la pipe actual*/
            if (i < cmdsTotal - 1) {
                close(p[0]);
                dup2(p[1], STDOUT_FILENO);
                close(p[1]);
            }

            execvp(cmds[i][0], cmds[i]);
            perror("Error al ejecutar comando");
            _exit(127);
        }

        //Proceso padre
        if (pids[i] > 0) {
        //Si ya se usó la pipe anterior (p[0]), se cierra
            if (entradaAc != STDIN_FILENO) {
                close(entradaAc);
            }

        /*Si no ha llegado al comando final, el padre cierra la copia
          de p[1] ya que no se usará y copia p[0] en entradaAc para
          conectarla con el siguiente comando*/
            if (i < cmdsTotal - 1) {
                close(p[1]);
                entradaAc = p[0];
            }
        }
    }

    //Espera a que terminen todos los procesos hijos
    for (int i = 0; i < cmdsTotal; i++) {
        waitpid(pids[i], NULL, 0);
    }

    //Libera la memoria de los pids de los procesos hijos
    free(pids);
}