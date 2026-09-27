#!/bin/bash
#Test para pipes

COMANDO="ls -1 | wc -l"

#La salida del comando ejecutado
SALIDA=$(echo "$COMANDO" | ../mishell)

#Assertions
if [[ "$SALIDA" =~ [0-9]+ ]]; then
    echo "Prueba exitosa"
    echo "Salida:"
    echo $SALIDA
    exit 0
else
    echo "Fallo en el test, la salida no contiene un número válido o el pipe falló"
    echo "Salida: '$SALIDA'"
    exit 1
fi