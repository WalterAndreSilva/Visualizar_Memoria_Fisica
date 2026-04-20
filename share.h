#include "conf.h"

#ifndef SHARE_H
#define SHARE_H

#define BUFFER_SIZE  (WIDTH*HEIGHT)+3
#define INDEX_VIEW    BUFFER_SIZE-1
#define INDEX_MODE    BUFFER_SIZE-2
#define INDEX_AKPS    BUFFER_SIZE-3

#define MASK_FREE (1<<0)
#define MASK_PGTB (1<<1)
#define MASK_RESE (1<<2)
#define MASK_COMP (1<<3)
#define MASK_FILE (1<<4)
#define MASK_ANON (1<<5)
#define MASK_USER (1<<6)
#define MASK_KERN (1<<7)
#define MASK_ALL   255

#define MAX_SCAN_GB MAX_RAM_SCAN_GB
#define V_SYNC V_SYNC_ACTIVE

// Codigo de color
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

static const float palette[11][4] = {
    {0.0, 0.0, 0.0, 1.0},
    {1.0, 0.0, 0.0, 1.0},
    {0.0, 0.0, 1.0, 1.0},
    {0.6, 1.0, 0.6, 1.0},
    {0.0, 0.3, 0.0, 1.0},
    {1.0, 0.8, 0.0, 1.0},
    {1.0, 0.0, 1.0, 1.0},
    {0.0, 0.8, 0.8, 1.0},
    {1.0, 0.4, 0.0, 1.0},
    {0.5, 0.0, 0.5, 1.0},
    {0.6, 0.3, 0.1, 1.0}
};

// seleccion de color para la vista
#define VAL_UNKN COL_NEGRO
#define VAL_VOID COL_ROJO
#define VAL_RESE COL_AZUL
#define VAL_PGTB COL_VERDE_OSCURO
#define VAL_COMP COL_AMARILLO
#define VAL_KERN COL_MAGENTA

#define VAL_FILE COL_NARANJA
#define VAL_ANON COL_MORADO
#define VAL_USER COL_MARRON
#define VAL_FREE COL_VERDE_CLARO

#define VAL_ZONE_DMA COL_MAGENTA
#define VAL_ZONE_DMA32 COL_CIAN
#define VAL_ZONE_NORMAL COL_AZUL

#define VAL_WRITEBACK COL_MORADO
#define VAL_DIRTY COL_AMARILLO

#endif
