# mishell — Shell simple con pipes, redirección y señales

Shell de texto simplificada para Linux, implementada en C (POSIX), como
Tarea 1 del curso Sistemas Operativos, 2026.

## Integrantes

- Tomás Garrido
- Martin Garcia
- Matias Pareja
- Bastian Cabezas

## Prerrequisitos

- `gcc` con soporte para `-std=gnu11`
- `make`
- `libreadline-dev` (necesaria para el historial de comandos navegable
  con las flechas del teclado). Instálala antes de compilar:

  ```bash
  sudo apt update && sudo apt install libreadline-dev
  ```

  Sin esta librería, `make` falla con `fatal error: readline/readline.h`.

## Compilación

```bash
make
```

Esto genera el ejecutable `mishell` en la raíz del proyecto, compilando
con `gcc -Wall -Wextra -std=gnu11` y enlazando con `-lreadline`.

Para limpiar los binarios generados:

```bash
make clean
```

## Ejecución

```bash
./mishell
```

Se abrirá el prompt `MiShell:<directorio actual>$`. Para salir:

```
exit
```

o `Ctrl+D`.

## Ejemplos de uso

```
MiShell:~$ ls -l | grep ".c" | wc -l
MiShell:~$ sort < datos.txt > datos_ordenados.txt
MiShell:~$ sleep 30 &
[1] 4821
MiShell:~$ jobs
[1] Ejecutando sleep 30
MiShell:~$ pmon 2
Monitoreando 1 proceso(s) cada 2 segundos...

   PID      COMANDO              ESTADO     %CPU         RSS(KB)
>> 4821     sleep                S          0.0          712
```

`pmon` se detiene con Ctrl+C, o solo cuando todos los procesos
monitoreados terminan.

### Historial de comandos (bonus)

`mishell` guarda un historial de los comandos escritos, navegable con
las flechas ↑/↓ del teclado, usando la librería
`readline`. El historial persiste entre sesiones en el archivo
`~/.mishell_history`.

## Estructura del proyecto

```
src/        código fuente (.c/.h) por módulo
tests/      pruebas automatizadas (ver sección Pruebas)
```

## Pruebas

En `tests/` hay pruebas automatizadas que cubren distintas partes de
la especificación. Algunas requieren compilar (`.c`), otras corren
directamente como scripts (`.sh`).

### Prerrequisitos para correrlas

- `bash` (viene instalado en cualquier distribución Linux)
- `python3` (usa únicamente el módulo `pty` de la librería estándar,
  no requiere instalar nada adicional con pip)

### Ciclo principal y comandos internos

- **`test_mishell.c`**

### Pipes y pmon

- **`pipesTest.c`**
- **`pmonTest.c`**

### Redirección y pipes (bash)

- **`test_redir.sh`** y **`test_pipes.sh`**

### Background y señales

- **`test_background.sh`**
- **`test_signals.py`**

### Cómo correrlas

Compila primero el proyecto (`make`, ver sección Compilación), y
luego, parado dentro de `tests/`:

> **Nota:** los comandos de abajo asumen que el código fuente vive en
> `../src/` (un nivel arriba de `tests/`). 
```bash
cd tests

# Scripts de bash / Python
bash test_background.sh ../mishell
python3 test_signals.py ../mishell
bash test_redir.sh
bash test_pipes.sh

# Pruebas en C (compilar primero cada una)
gcc -Wall -Wextra -std=gnu11 -o test_mishell test_mishell.c
./test_mishell

gcc -Wall -Wextra -std=gnu11 -o pipesTest pipesTest.c ../src/pipes.c
./pipesTest

gcc -Wall -Wextra -std=gnu11 -o pmonTest pmonTest.c
./pmonTest <PID>

gcc -Wall -Wextra -std=gnu11 -o test_pmon test_pmon.c ../src/pmon.c
./test_pmon <PID>
```

Cada prueba imprime `[OK]` o `[FALLO]` por cada caso, y un resumen
final, por ejemplo:

```
== Resumen R5: 5 OK / 0 FALLOS ==
== Resumen R6: 3 OK / 0 FALLOS ==
```
