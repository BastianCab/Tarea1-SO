#ifndef HISTORY_H
#define HISTORY_H

/* Inicializa el historial de comandos: intenta cargar el historial
 * previo desde ~/.mishell_history (si existe), para que persista
 * entre sesiones, igual que hace bash. Debe llamarse UNA vez, al
 * inicio de main(), antes del ciclo principal. */
void history_init(void);

/* Lee una línea del usuario usando readline(), con edición completa
 * (flechas arriba/abajo para navegar el historial, Ctrl+A/Ctrl+E,
 * Ctrl+R para buscar, etc.). 'prompt' es el prompt a mostrar (ej.
 * "MiShell:/home/user$ ").
 *
 * Devuelve un puntero a una cadena reservada con malloc() por
 * readline() -- el llamador es responsable de liberarla con free()
 * cuando termine de usarla. Devuelve NULL si el usuario presionó
 * Ctrl+D (EOF), igual que fgets(). */
char *history_leer_linea(const char *prompt);

/* Agrega 'linea' al historial EN MEMORIA (para las flechas), y
 * además la guarda en el archivo ~/.mishell_history para que
 * persista en la próxima sesión. No hace nada si 'linea' está
 * vacía o es idéntica a la última guardada (para no llenar el
 * historial de líneas repetidas consecutivas, igual que bash). */
void history_agregar(const char *linea);

/* Guarda el historial completo a disco. Se llama normalmente al
 * salir de la shell (built-in "exit" o Ctrl+D), aunque
 * history_agregar() ya va guardando incrementalmente. */
void history_guardar(void);

#endif /* HISTORY_H */