#!/bin/bash
#Test para redirections

ARCHIVO_TEST="test_redir.txt"

#Para comprobar que el entorno esté limpio
rm -f "$ARCHIVO_TEST"

#Test 1 para la redirección de salida normal (>)
echo "Ejecutando echo 'primera linea' > $ARCHIVO_TEST"
TEST=$(echo 'primera linea' > $ARCHIVO_TEST | ../mishell)

if [ ! -f "$ARCHIVO_TEST" ]; then
    echo "Error, no se creó el archivo de salida"
    exit 1
fi

CONTENIDO_1=$(cat "$ARCHIVO_TEST")
if [[ "$CONTENIDO_1" == *"primera linea"* ]]; then
    echo "  Test 1 redirección de salida normal exitoso"
    echo ""
else
    echo "  El test 1 falló, el archivo se creó, pero el contenido es incorrecto: '$CONTENIDO_1'"
    exit 1
fi

#Test 2 para la redirección de salida append (>>)
echo "Ejecutando echo 'segunda linea' >> $ARCHIVO_TEST"
TEST=$(echo 'segunda linea' >> $ARCHIVO_TEST | ../mishell)

CONTENIDO_2=$(cat "$ARCHIVO_TEST")
if [[ "$CONTENIDO_2" == *"primera linea"* ]] && [[ "$CONTENIDO_2" == *"segunda linea"* ]]; then
    echo "  Test 2 redirección de salida append exitoso"
    echo ""
else
    echo "  El test falló, el texto no se agregó correctamente"
    echo "  Contenido actual: '$CONTENIDO_2'"
    exit 1
fi

#Test 3 redirección de entrada (<)
echo "Ejecutando cat < $ARCHIVO_TEST"
#Se usa cat sin argumentos, entonces va a leer desde la redirección de STDIN
SALIDA_ENTRADA=$(echo "cat < $ARCHIVO_TEST" | ../mishell)

if [[ "$SALIDA_ENTRADA" == *"primera linea"* ]] && [[ "$SALIDA_ENTRADA" == *"segunda linea"* ]]; then
    echo "  Test 3 redirección de entrada exitoso"
else
    echo "  El test falló, error en la lectura desde stdin"
    echo "  Salida actual: '$SALIDA_ENTRADA'"
    exit 1
fi

rm -f "$ARCHIVO_TEST"
echo ""
echo "Todos los tests funcionaron correctamente"
exit 0