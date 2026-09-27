#!/bin/bash
#
# test_background.sh ; Pruebas de R5
#
# Verifica:
#   1. Al lanzar "cmd &", se imprime de inmediato "[N] PID".
#   2. El comando "jobs" lista los procesos en background activos.
#   3. Al terminar un job, se notifica "[N]+ Done <cmd>" antes del
#      siguiente prompt.
#   4. No quedan procesos zombie después de que los jobs terminan.
#   5. Varios jobs lanzados casi simultáneamente se recolectan TODOS
#      (valida el ciclo waitpid(-1, &status, WNOHANG) del manejador
#      de SIGCHLD, no solo una llamada).
#
# Uso:
#   ./test_background.sh ../mishell
#

set -u

MISHELL="${1:-../mishell}"
PASS=0
FAIL=0

verde() { printf "\033[32m%s\033[0m\n" "$1"; }
rojo()  { printf "\033[31m%s\033[0m\n" "$1"; }

check() {
    local descripcion="$1"
    local condicion="$2"
    if [ "$condicion" = "1" ]; then
        verde "  [OK] $descripcion"
        PASS=$((PASS+1))
    else
        rojo "  [FALLO] $descripcion"
        FAIL=$((FAIL+1))
    fi
}

if [ ! -x "$MISHELL" ]; then
    rojo "No se encontró el ejecutable '$MISHELL'. Compila primero con 'make'."
    exit 1
fi

echo "== R5: Ejecución en background =="
echo "Binario bajo prueba: $MISHELL"
echo

# Prueba 1: se imprime "[N] PID" de inmediato al lanzar un job 
echo "-- Prueba 1: aviso inmediato [N] PID --"
SALIDA1=$(printf 'sleep 1 &\nexit\n' | "$MISHELL" 2>&1)
if echo "$SALIDA1" | grep -qE '\[1\] [0-9]+'; then R1=1; else R1=0; fi
check "Se imprimió '[1] <PID>' al lanzar 'sleep 1 &'" "$R1"
echo

# Prueba 2: comando "jobs" lista el proceso en ejecución
echo "-- Prueba 2: comando 'jobs' --"
SALIDA2=$(printf 'sleep 2 &\njobs\nexit\n' | "$MISHELL" 2>&1)
if echo "$SALIDA2" | grep -qi 'sleep 2'; then R2=1; else R2=0; fi
check "'jobs' muestra el comando en ejecución (sleep 2)" "$R2"
echo

# Prueba 3: notificación "Done" tras terminar 
echo "-- Prueba 3: notificación de término --"
SALIDA3=$(printf 'sleep 1 &\nsleep 2\nexit\n' | "$MISHELL" 2>&1)
if echo "$SALIDA3" | grep -qi 'Done'; then R3=1; else R3=0; fi
check "Se notificó 'Done' tras terminar el job en background" "$R3"
echo

# Prueba 4: sin procesos zombie
echo "-- Prueba 4: ausencia de procesos zombie --"
(printf 'sleep 1 &\nsleep 1 &\nsleep 1 &\nsleep 3\nexit\n' | "$MISHELL" > /dev/null 2>&1) &
SHELL_TEST_PID=$!
sleep 4
ZOMBIES=$(ps -o pid,stat,cmd --ppid "$SHELL_TEST_PID" 2>/dev/null | grep -c 'Z')
wait "$SHELL_TEST_PID" 2>/dev/null
if [ "${ZOMBIES:-0}" -eq 0 ]; then R4=1; else R4=0; fi
check "No quedaron procesos zombie tras 3 jobs en background" "$R4"
echo

# Prueba 5: múltiples jobs terminando casi simultáneamente
echo " Prueba 5: recolección de múltiples SIGCHLD simultáneos "
SALIDA5=$(printf 'sleep 1 &\nsleep 1 &\nsleep 2\nexit\n' | "$MISHELL" 2>&1)
DONE_COUNT=$(echo "$SALIDA5" | grep -c 'Done')
if [ "$DONE_COUNT" -ge 2 ]; then R5=1; else R5=0; fi
check "Se notificaron los 2 jobs terminados casi al mismo tiempo (encontrados: $DONE_COUNT)" "$R5"
echo

echo "== Resumen R5: $PASS OK / $FAIL FALLOS =="
[ "$FAIL" -eq 0 ] && verde "¡Todas las pruebas pasaron!" || rojo "Algunas pruebas fallaron."