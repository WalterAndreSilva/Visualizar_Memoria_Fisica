#ifndef CONF_H
#define CONF_H

// --- AJUSTES PARA 16 GB DE RAM ---
// 16 GB = 4,194,304 páginas.
// 2048x2048 = 4,194,304 píxeles.
#define WIDTH 2048
#define HEIGHT 2048

// Escaneo de páginas fisicas que pertenecen a la RAM
// Tiene que ser superior al tamaño real (16 GB)
// Hay páginas fisicas que no estan en la RAM.
#define MAX_RAM_SCAN_GB 32ULL

// V-SYNC: 0 = desactivado, 1 = activado (max 60fps generalmente)
#define V_SYNC_ACTIVE 1

#endif
