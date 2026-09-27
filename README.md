# mishell — Shell simple con pipes, redirección y señales

Shell de texto simplificada para Linux, implementada en C (POSIX), como
Tarea 1 del curso Sistemas Operativos, 2026.

## Integrantes

- Tomás Garrido
- Martin Garcia
- Matias Pareja
- Bastian Cabezas

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

En `tests/` hay dos scripts que prueban automáticamente R5 (background)
y R6 (manejo de señales). No requieren compilación (son scripts, no
código C), pero sí estos prerrequisitos:

- `bash` (viene instalado en cualquier distribución Linux)
- `python3` (usa únicamente el módulo `pty` de la librería estándar,
  no requiere instalar nada adicional con pip)

### Cómo correrlos

Compila primero el proyecto (`make`, ver sección Compilación), y luego:

```bash
cd tests
bash test_background.sh ../mishell      # prueba R5
python3 test_signals.py ../mishell      # prueba R6
```

Cada uno imprime `[OK]` o `[FALLO]` por cada caso probado, y un resumen
final, por ejemplo:

== Resumen R5: 5 OK / 0 FALLOS ==

== Resumen R6: 3 OK / 0 FALLOS ==
