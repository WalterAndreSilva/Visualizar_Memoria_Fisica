#ifndef CONF_H
#define CONF_H

// --- AJUSTES PARA 16 GB DE RAM ---
// 16 GB = (16*2^30)/4096 = 4,194,304 páginas.
// 2048x2048 = 4,194,304 píxeles.
#define WIDTH 2048
#define HEIGHT 2048

// Escaneo de páginas fisicas que pertenecen a la RAM
// Tiene que ser superior al tamaño real (16 GB)
// Hay PFN en direcciones fisicas superiores a la RAM teorica.
#define MAX_RAM_SCAN_GB 32ULL

// V-SYNC: 0 = desactivado, 1 = activado (max 60fps generalmente)
#define V_SYNC_ACTIVE 1

// Cantidad maxima de acualizaciones del kernel por segundo
// Valor minimo = 1
#define MAX_UPDATE_KERN_SEC 100

// Captura de video
// WARNING: Para realizar la capura se crea un pipe con ffmpeg
// Durante la grabacion se desactiva la opcion de ventana
// No = 0, Si = 1
#define CAPTURE_VIDEO 0

// Tope de fps para sincronisar openGL con ffmpeg.
// Si no se consigue la cantidad de fps necesarias, el video
// puede quedar acelerado
#define TARGET_FPS_REC 30

#endif
