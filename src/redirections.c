#include "redirections.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

/*Funcion que redirecciona la salida y/o entrada de archivos.
  archivoEntrada: El nombre del archivo para
                  redireccionar la entrada (<), dejar NULL si no hay
  archivoSalida: El nombre del archivo para redireccionar
                 la salida (>), dejar NULL si no hay
  append: Un verificador si el archivo de salida usa O_APPEND u O_TRUNC*/
void redireccionar(char* archivoEntrada, char* archivoSalida, int append) {
    if (archivoEntrada != NULL) {

        //File descriptor para el archivo de entrada
        int fdIn = open(archivoEntrada, O_RDONLY);
        if (fdIn == -1) {
            perror("Error al abrir archivo de entrada");
            _exit(1);
        }

        dup2(fdIn, STDIN_FILENO);
        close(fdIn);
    }

    if (archivoSalida != NULL) {
        int flags = O_WRONLY | O_CREAT;
        
        //File descriptor del archivo de salida, se inicializa segun append
        int fdOut;

        if(append){
            fdOut= open(archivoSalida, flags | O_APPEND, 0644);
        }
        else{
            fdOut= open(archivoSalida, flags | O_TRUNC, 0644);
        }

        if(fdOut== -1){
            perror("Error al abrir archivo de salida");
            return;
        }
        if(dup2(fdOut, STDOUT_FILENO)== -1){
            perror("Error en dup2");
            close(fdOut);
            return;
        }

        close(fdOut);
    }
}