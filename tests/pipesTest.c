#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>

void creaPipe(char*** cmds, int cmdsTotal);

int main() {
    //Guarda la salida estándar para reestablecerla después
    int stdoutOriginal = dup(STDOUT_FILENO);

    //Crea y abre un archivo temporal para la salida del pipe
    int fdTemp = open("test_output.txt", O_RDWR | O_CREAT | O_TRUNC, 0666);
    assert(fdTemp != -1 && "Error al crear el archivo temporal");

    //Conecta stdout al archivo temporal
    dup2(fdTemp, STDOUT_FILENO);

    //Prepara los comandos para probar
    char *cmd1[] = {"ls", "-1", NULL};
    char *cmd2[] = {"wc", "-l", NULL};
    
    //El arreglo de los comandos
    char **cmds[] = {cmd1, cmd2};
    creaPipe((char***)cmds, 2);

    //Para asegurar que los procesos hayan terminado
    fflush(stdout);

    //Reestablece el stdout original
    dup2(stdoutOriginal, STDOUT_FILENO);
    close(stdoutOriginal);

    //Lee el contenido del archivo temporal
    lseek(fdTemp, 0, SEEK_SET);
    char buffer[128] = {0};
    ssize_t leerBytes = read(fdTemp, buffer, sizeof(buffer) - 1);
    close(fdTemp);
    remove("test_output.txt");

    assert(leerBytes > 0 && "El pipe no generó salida");

    int digito = 0;
    for (int i = 0; i < leerBytes; i++) {
        if (isdigit(buffer[i])) {
            digito = 1;
            break;
        }
    }

    assert(digito == 1 && "El salida no contiene un formato de wc");
    return 0;
}