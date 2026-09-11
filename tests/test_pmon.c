#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include "../src/pmon.h"


int main(int argc, char *argv[]){
    if(argc< 2){
        printf("Uso: %s PID1 [PID2 PID3 ...]\n", argv[0]);
        return 1;
    }

    int cantidad= argc - 1;

    pid_t *pids= calloc(cantidad, sizeof(pid_t));

    if(pids== NULL){
        fprintf(stderr, "Error al reservar memoria\n");
        return 1;
    }

    for(int i= 0; i< cantidad; i++){
        pids[i]= (pid_t)atoi(argv[i + 1]);
    }

    int resultado= ejecutar_pmon(pids, cantidad, 2);

    free(pids);

    return resultado;
}