/*
 * history.c — Bonus: historial de comandos navegable con las flechas
 * del teclado, usando la librería GNU readline.
 *
 * Por qué readline y no una implementación manual:
 *   Para que las flechas ↑/↓ funcionen, hay que poner la terminal en
 *   modo "raw" (sin buffer de línea ni eco automático del kernel),
 *   leer byte por byte, y reconocer manualmente las secuencias de
 *   escape que manda el teclado (ej. flecha arriba = ESC '[' 'A').
 *   Reimplementar todo eso a mano es exactamente lo que readline ya
 *   resuelve de fábrica -- además de dar edición de línea completa
 *   (Ctrl+A/Ctrl+E para moverse al inicio/fin, Ctrl+R para buscar en
 *   el historial, etc.), que casi todas las shells reales (bash,
 *   zsh) usan tal cual.
 *
 * IMPORTANTE para compilar: esto requiere linkear con -lreadline.
 * En Ubuntu/Debian, si no está instalado:
 *   sudo apt install libreadline-dev
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "history.h"

#define HISTORY_MAX_ENTRADAS 500

/* Construye la ruta ~/.mishell_history. Devuelve un puntero a un
 * buffer estático (suficiente para este uso: se llama solo una vez
 * por operación, no hay necesidad de reentrancia). */
static const char *history_ruta_archivo(void) {
    static char ruta[1024];
    const char *home = getenv("HOME");
    if (home == NULL) {
        // Sin $HOME no hay dónde persistir; se usa un nombre relativo
        // como último recurso (el historial en memoria seguirá
        // funcionando dentro de la sesión de todas formas).
        snprintf(ruta, sizeof(ruta), ".mishell_history");
    } else {
        snprintf(ruta, sizeof(ruta), "%s/.mishell_history", home);
    }
    return ruta;
}

void history_init(void) {
    using_history(); // inicializa las estructuras internas de readline

    // Intentar cargar historial previo. read_history() devuelve
    // distinto de 0 si el archivo no existe todavía (ej. primera vez
    // que se usa mishell) -- no es un error real, simplemente no hay
    // nada que cargar aún.
    read_history(history_ruta_archivo());

    // Limitar cuántas líneas se guardan, para que el archivo no
    // crezca indefinidamente con el tiempo.
    stifle_history(HISTORY_MAX_ENTRADAS);
}

char *history_leer_linea(const char *prompt) {
    // readline() imprime el prompt, maneja toda la edición de línea
    // (flechas, Ctrl+A/E, etc.) y devuelve la línea completa SIN el
    // '\n' final (a diferencia de fgets()). Devuelve NULL en EOF
    // (Ctrl+D), igual semántica que fgets() devolviendo NULL.
    return readline(prompt);
}

void history_agregar(const char *linea) {
    if (linea == NULL || linea[0] == '\0') {
        return; // no guardar líneas vacías
    }

    // Evitar duplicar la MISMA línea si el usuario la repite
    // consecutivamente (igual que el comportamiento por defecto de
    // bash con HISTCONTROL=ignoredups).
    HIST_ENTRY *ultima = history_get(history_length);
    if (ultima != NULL && strcmp(ultima->line, linea) == 0) {
        return;
    }

    add_history(linea);

    // Persistir incrementalmente: se agrega solo la última línea al
    // archivo, en vez de reescribirlo completo cada vez (más barato
    // si el historial ya es largo).
    append_history(1, history_ruta_archivo());
}

void history_guardar(void) {
    // Reescribe el archivo completo con el historial actual en
    // memoria. Se llama típicamente al salir de la shell, como
    // respaldo (append_history() ya se encarga del caso normal).
    write_history(history_ruta_archivo());
}