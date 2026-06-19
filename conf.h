#ifndef CONF_H
#define CONF_H

// --- AJUSTES PARA 16 GB DE RAM ---
// 16 GB = (16*2^30)/4096 = 4 194 304 páginas.
// 2048x2048 = 4 194 304 píxeles.
#define WIDTH 2048
#define HEIGHT 2048

// Escaneo de páginas fisicas que pertenecen a la RAM
// Tiene que ser superior al tamaño real
// Hay PFN en direcciones fisicas superiores a la RAM teórica.
#define MAX_RAM_SCAN_GB 32L

// Cantidad máxima de actualizaciones del kernel por segundo
// Valor mínimo = 1
#define MAX_UPDATE_KERN_SEC 30

// Captura de video
// WARNING: Para realizar la captura se crea un pipe con ffmpeg
// Durante la grabación se desactiva la opción de ventana
// No = 0, Si = 1
#define CAPTURE_VIDEO 0

// Tope de fps para sincronizar openGL con ffmpeg.
// Si no se consigue la cantidad de fps necesarias, el video
// puede quedar acelerado
#define TARGET_FPS_REC 30

// Forzar que la ventana tenga el tamaño de la textura.
// Con esta opción se obtiene una relación 1:1 pagina pixel
// Se desactivan la visualización del menú de opciones.
#define FORCE_WINDOWS_TO_TEXTURE 0

#endif
