#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>

#include "jobs.h"
#include "pmon.h"
#include "pipes.h"
#include "signals.h"
#include "redirections.h"

#define MAXIM 1024

// Declarar el arreglo global definido en jobs.c para poder extraer los PIDs
extern job_t jobs[MAX_JOBS];

int main() {
    char input[MAXIM];
    char input_copy[MAXIM];
    char *args[100];
    char cwd[1024];

    shell_signals_init();
    jobs_init();

    while (1) {
        
        // Revisamos si algún proceso de fondo terminó y avisamos
        jobs_notify_done(); 

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
    
        // Respaldar la frase original antes de separarla para poder registrarla en la lista de jobs
        strncpy(input_copy, input, MAXIM);

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
        
        // Implementamos el comando 'jobs'
        } else if (strcmp("jobs", args[0]) == 0) {
            jobs_print_list();
        
        // Implementamos el comando 'pmon'
        } else if (strcmp("pmon", args[0]) == 0) {
            // Configuramos el intervalo de actualización (por defecto 2 segundos si se omite)
            int intervalo = 2;
            if (args[1] != NULL) {
                intervalo = atoi(args[1]);
            }

            // Extraer los PIDs de los procesos activos iterando sobre la estructura global de jobs
            pid_t pids_activos[MAX_JOBS];
            int cantidad_pids = 0;
            
            for (int j = 0; j < MAX_JOBS; j++) {
                if (jobs[j].in_use) {
                    pids_activos[cantidad_pids] = jobs[j].pid;
                    cantidad_pids++;
                }
            }

            // Ejecutar el monitor pmon solo si existen procesos en background válidos
            if (cantidad_pids > 0) {
                ejecutar_pmon(pids_activos, cantidad_pids, intervalo);
            } else {
                printf("No hay procesos en background para monitorear.\n");
            }
        
        // COMANDOS EXTERNOS 
        // Gestionamos la ejecución de programas del sistema si el comando no es interno
        } else {   
            
            // Detectar si el usuario incluyó el operador '&' al final para ejecución en background
            int background = 0;
            int ultimo_indice = i - 1;
            
            if (ultimo_indice >= 0 && strcmp(args[ultimo_indice], "&") == 0) {
                background = 1;
                args[ultimo_indice] = NULL; // Remover el símbolo '&' de los argumentos para no interferir con la ejecución
            }

            // Generamos un proceso hijo para delegar la ejecución del comando y evitar que la shell original se muera
            pid_t pid = fork();
            
            if (pid == 0) {
                // Restaurar las señales por defecto para que Ctrl+C pueda matar al comando en foreground
                child_signals_restore_default();
                
                // FALTA IMPLEMENTAR LA REDIRECCION DE ENTRADA Y SALIDA SI EL USUARIO INCLUYE '>' O '<'

                // execvp borra el cerebro a este clon y lo reemplaza por el programa que pidió el usuario (ej: ls)
                if (execvp(args[0], args) == -1) {
                    perror("Error al ejecutar comando"); // Notificamos si el comando es inválido o no existe
                    exit(1);                             // Finalizamos el proceso hijo en caso de error
                }
            
            } else if (pid > 0) {
                
                if (background) {
                    // Registrar el proceso en la lista de jobs si es asíncrono y omitir el bloqueo del padre
                    jobs_add(pid, input_copy);
                } else {
                    // Suspendemos la ejecución de la shell con waitpid, hasta que el proceso hijo haya concluido
                    waitpid(pid, NULL, 0);
                }
            
            } else {
                perror("Error al crear proceso hijo"); // Manejamos los posibles fallos de clonación en el sistema
            }
        }
    }

    return 0;
}