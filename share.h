#include "conf.h"

#ifndef SHARE_H
#define SHARE_H

#define BUFFER_SIZE (WIDTH*HEIGHT)+2
#define INDEX_MODE   BUFFER_SIZE-1
#define INDEX_AKPS   BUFFER_SIZE-2


#define MAX_SCAN_GB MAX_RAM_SCAN_GB
#define V_SYNC V_SYNC_ACTIVE

// codigo de color definido en el shader
typedef enum {
    COL_NEGRO,
    COL_ROJO,
    COL_AZUL,
    COL_VERDE_CLARO,
    COL_VERDE_OSCURO,
    COL_AMARILLO,
    COL_MAGENTA,
    COL_CIAN,
    COL_NARANJA,
    COL_MORADO,
    COL_MARRON
} ColorMemory;

// seleccion de color para la vista
#define VAL_UNKN COL_NEGRO
#define VAL_VOID COL_ROJO
#define VAL_RESE COL_AZUL
#define VAL_PGTB COL_VERDE_OSCURO
#define VAL_COMP COL_AMARILLO

#define VAL_FILE COL_NARANJA
#define VAL_ANON COL_MORADO
#define VAL_USER COL_MARRON
#define VAL_FREE COL_VERDE_CLARO

#define VAL_ZONE_DMA COL_MAGENTA
#define VAL_ZONE_DMA32 COL_CIAN
#define VAL_ZONE_NORMAL COL_NARANJA

#endif
