#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>

int main() {
    int ok = 0;
    int fallos = 0;

    printf("Iniciando pruebas del ciclo principal (mishell.c)...\n\n");

    // COMANDO EXTERNO SIMPLE echo
    
    // Creamos unos archivos temporales para simular la terminal y atrapar lo que imprime la shell
    int fdIn1 = open("test_in1.txt", O_RDWR | O_CREAT | O_TRUNC, 0666);
    int fdOut1 = open("test_out1.txt", O_RDWR | O_CREAT | O_TRUNC, 0666);

    // Dejamos los comandos listos y los metemos en el archivo de entrada
    char *cmds1 = "echo validacion_mishell\nexit\n";
    write(fdIn1, cmds1, strlen(cmds1));
    
    // Devolvemos el cursor al inicio para que la shell lea desde el principio
    lseek(fdIn1, 0, SEEK_SET); 

    // Generamos un proceso hijo para aislar la shell
    pid_t pid1 = fork();
    if (pid1 == 0) {
        // Reemplazamos la entrada y salida de este clon por nuestros archivos
        dup2(fdIn1, STDIN_FILENO);
        dup2(fdOut1, STDOUT_FILENO);
        close(fdIn1);
        close(fdOut1);

        // Ejecutamos la shell para que pise a este proceso hijo
        execl("../mishell", "mishell", NULL); 
        exit(1); // Si llega acá es porque falló el execl
    }

    // Suspendemos al padre hasta que la shell termine su trabajo
    waitpid(pid1, NULL, 0);

    // Leemos todo el contenido que dejó la shell en nuestro archivo de salida
    lseek(fdOut1, 0, SEEK_SET);
    char buffer1[2048] = {0};
    ssize_t leerBytes1 = read(fdOut1, buffer1, sizeof(buffer1) - 1);
    
    close(fdIn1);
    close(fdOut1);
    
    // Borramos los archivos temporales para no ensuciar la carpeta
    remove("test_in1.txt");
    remove("test_out1.txt");

    // Verificamos si la shell nos devolvió la palabra que esperábamos
    if (leerBytes1 > 0 && strstr(buffer1, "validacion_mishell") != NULL) {
        printf("[OK] Comando externo simple (echo)\n");
        ok++;
    } else {
        printf("[FALLO] Comando externo simple (echo)\n");
        fallos++;
    }

    // COMANDO INTERNO cd Y LINEAS VACIAS
    
    int fdIn2 = open("test_in2.txt", O_RDWR | O_CREAT | O_TRUNC, 0666);
    int fdOut2 = open("test_out2.txt", O_RDWR | O_CREAT | O_TRUNC, 0666);

    // Simulamos que el usuario aprieta Enter varias veces sin escribir nada y luego hace cd
    char *cmds2 = "\n\ncd /tmp\npwd\nexit\n";
    write(fdIn2, cmds2, strlen(cmds2));
    lseek(fdIn2, 0, SEEK_SET);

    pid_t pid2 = fork();
    if (pid2 == 0) {
        dup2(fdIn2, STDIN_FILENO);
        dup2(fdOut2, STDOUT_FILENO);
        close(fdIn2);
        close(fdOut2);

        execl("../mishell", "mishell", NULL);
        exit(1);
    }

    waitpid(pid2, NULL, 0);

    lseek(fdOut2, 0, SEEK_SET);
    char buffer2[2048] = {0};
    ssize_t leerBytes2 = read(fdOut2, buffer2, sizeof(buffer2) - 1);
    
    close(fdIn2);
    close(fdOut2);
    remove("test_in2.txt");
    remove("test_out2.txt");

    // Buscamos /tmp en la salida para confirmar que el built-in cd funcionó bien
    if (leerBytes2 > 0 && strstr(buffer2, "/tmp") != NULL) {
        printf("[OK] Comando interno (cd) y manejo de lineas vacias\n");
        ok++;
    } else {
        printf("[FALLO] Comando interno (cd) y manejo de lineas vacias\n");
        fallos++;
    }
 
    // RESULTADOS FINALES

    printf("\n== Resumen MAIN: %d OK / %d FALLOS ==\n", ok, fallos);

    return (fallos == 0) ? 0 : 1;
}