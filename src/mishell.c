#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#define MAXIM 1024

int main() {
    char input[MAXIM];
    char *args[100];
    char cwd[1024];

    while (1) {
        
        // ANALISIS E IMPRESION DEL DIRECTORIO ACTUAL
        // Obtenemos la ruta del directorio de trabajo actual
        // Imprimios el prompt de la shell usando el directorio obtenido
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("MiShell:%s$ ", cwd);
        } else {
            perror("Error al obtener directorio");
        }


        // LECTURA DE LA ENTRADA DE USUARIO
        // Se usa fgets para atrapar lo que se escribe en la consola.
        // Si devuelve NULL es porque se apretó Ctrl+D, así que rompemos el ciclo para salir
        if (fgets(input,MAXIM,stdin) == NULL) {
            printf("\n");
            break;
        } else {
            // Eliminar el carácter de salto de línea (\n) almacenado por fgets al final del texto
            // Se busca el salto de línea y se reemplaza por un carácter nulo para limpiar la cadena
            input[strcspn(input, "\n")] = 0;
            
            // Si se ingresa una línea vacía (solo Enter), reiniciamos el ciclo
            if (strlen(input) == 0) {
                continue;
            }
        }
    

        // DIVISION DE LA ORACION EN PALABRAS
        // Separamos la frase completa ingresada por el usuario usando los espacios como separador
        // Se almacena cada argumento en el arreglo 'args' para su posterior ejecución
        int i = 0;
        args[i] = strtok(input, " ");
        while (args[i] != NULL) {
            i++;
            args[i] = strtok(NULL, " ");
        }
        // Asignar NULL al final del arreglo para que la función execvp pueda saber dónde termina la lista de argumentos
        args[i] = NULL;


        // COMANDOS INTERNOS
        // Gestionamos los comandos que deben ser ejecutados por el proceso padre directamente
        
        // Finalizar la ejecución de la shell.
        if (strcmp("exit", args[0]) == 0) {
            break;
        
        // Modificar el directorio de trabajo actual
        } else if (strcmp("cd", args[0]) == 0) {
            if (args[1] != NULL) {
                chdir(args[1]);
            }
        
        // AQUI SE DEBE IMPLEMENTAR EL COMANDO 'jobs'
        } else if (strcmp("jobs", args[0]) == 0) {
            
        
        // AQUI SE DEBE IMPLEMENTAR EL COMANDO 'pmon'
        } else if (strcmp("pmon", args[0]) == 0) {
            
        
        // COMANDOS EXTERNOS 
        // Gestionamos la ejecución de programas del sistema si el comando no es interno
        
        } else {
            // Generamos un proceso hijo para delegar la ejecución del comando y evitar que la shell original se muera
            pid_t pid = fork();
            
            if (pid == 0) {
                // execvp borra el cerebro a este clon y lo reemplaza por el programa que pidió el usuario (ej: ls)
                if (execvp(args[0], args) == -1) {
                    perror("Error al ejecutar comando"); // Notificamos si el comando es inválido o no existe
                    exit(1);                             // Finalizamos el proceso hijo en caso de error
                }
            
            } else if (pid > 0) {
                // Suspendemos la ejecución de la shell (proceso padre) con waipid, hasta que el proceso hijo haya concluido
                waitpid(pid, NULL, 0);
            
            } else {
                perror("Error al crear proceso hijo"); // Manejamos los posibles fallos de clonación en el sistema
            }
        }
    }

    return 0;
}