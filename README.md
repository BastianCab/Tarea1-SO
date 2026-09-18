# mishell — Shell simple con pipes, redirección y señales

Shell de texto simplificada para Linux, implementada en C (POSIX), como
Tarea 1 del curso Sistemas Operativos, 2026.

## Integrantes

- Tomás Garrido
- Martin Garcia
- Matias Pareja
- Bastian

## Compilación

```bash
make
```

Esto genera el ejecutable `mishell` en la raíz del proyecto, compilando
con `gcc -Wall -Wextra -std=gnu11`.

Para limpiar los binarios generados:

```bash
make clean
```

## Ejecución

```bash
./mishell
```

Se abrirá el prompt `miShell:<directorio actual>$`. Para salir:

```
exit
```

o `Ctrl+D`.

## Ejemplos de uso

```
miShell:~$ ls -l | grep ".c" | wc -l
miShell:~$ sort < datos.txt > datos_ordenados.txt
miShell:~$ sleep 30 &
[1] 4821
miShell:~$ jobs
[1] Ejecutando sleep 30
miShell:~$ pmon 2
```

## Estructura del proyecto

```
src/        código fuente (.c/.h) por módulo
tests/      scripts de prueba manual
informe/    informe corto en PDF
docs/       diagramas de arquitectura
```

## Pruebas

Ver `tests/`. Cada script `.sh` ejercita una parte de la especificación
(pipes, redirección, background, señales).

## Limitaciones conocidas

- (completar durante el desarrollo)
