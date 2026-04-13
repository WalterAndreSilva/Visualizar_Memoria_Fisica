#ifndef CONF_H
#define CONF_H

// --- AJUSTES PARA 16 GB DE RAM ---
// 16 GB = 4,194,304 páginas.
// 2048x2048 = 4,194,304 píxeles.
#define WIDTH 2048
#define HEIGHT 2048
// Sumamos 1 para enviar las actualizaciones del kernel por segundo
#define BUFFER_SIZE (WIDTH*HEIGHT)+1

// Escaneo de páginas fisicas que pertenecen a la RAM
// Tiene que ser superior al tamaño real (16 GB)
// Hay páginas fisicas que no estan en la RAM.
#define MAX_SCAN_GB 32ULL

// Visualización: Tamaño de la ventana
#define WIN_WIDTH 600
#define WIN_HEIGHT 600

// V-SYNC: 0 = desactivado, 1 = activado (max 60fps generalmente)
#define V_SYNC 1

#endif
