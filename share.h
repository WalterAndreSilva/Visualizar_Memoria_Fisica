#include "conf.h"

#ifndef SHARE_H
#define SHARE_H

#define TEXTURE_SIZE  (WIDTH*HEIGHT)
#define BUFFER_SIZE   (TEXTURE_SIZE+12)
#define INDEX_VIEW    (BUFFER_SIZE-2)        // uint16_t
#define INDEX_MODE    (BUFFER_SIZE-3)        // uint8_t
#define INDEX_AKPS    (BUFFER_SIZE-4)        // uint8_t
#define INDEX_TOTAL_PAGES (BUFFER_SIZE-12)   // uint64_t

#define MASK_FREE (1<<0)
#define MASK_RESE (1<<1)
#define MASK_COMP (1<<2)
#define MASK_SLAB (1<<3)
#define MASK_PGTB (1<<4)
#define MASK_FILE (1<<12)
#define MASK_ANON (1<<13)
#define MASK_USER (1<<14)
#define MASK_KERN (1<<15)

#define MASK_ALL  ((uint16_t)~0)

#define MAX_SCAN_GB MAX_RAM_SCAN_GB
#define V_SYNC V_SYNC_ACTIVE

// Codigo de color
typedef enum {
    COL_BLACK,
    COL_RED,
    COL_BLUE,
    COL_LIGHT_GREEN,
    COL_DARK_GREEN,
    COL_YELLOW,
    COL_MAGENTA,
    COL_CYAN,
    COL_ORANGE,
    COL_PURPLE,
    COL_BROWN,
    COL_PINK,
    COL_SKY,
    COL_GREY,
    COL_LIME,
    COL_TEAL
} ColorMemory;

static const float palette[16][4] = {
    {0.0, 0.0, 0.0, 1.0}, // COL_BLACK
    {1.0, 0.0, 0.0, 1.0}, // COL_RED
    {0.0, 0.0, 1.0, 1.0}, // COL_BLUE
    {0.6, 1.0, 0.6, 1.0}, // COL_LIGHT_GREEN
    {0.0, 0.3, 0.0, 1.0}, // COL_DARK_GREEN
    {1.0, 0.8, 0.0, 1.0}, // COL_YELLOW
    {1.0, 0.0, 1.0, 1.0}, // COL_MAGENTA
    {0.0, 0.8, 0.8, 1.0}, // COL_CYAN
    {1.0, 0.4, 0.0, 1.0}, // COL_ORANGE
    {0.5, 0.0, 0.5, 1.0}, // COL_PURPLE
    {0.6, 0.3, 0.1, 1.0}, // COL_BROWN
    {1.0, 0.1, 0.6, 1.0}, // COL_PINK
    {0.3, 0.6, 1.0, 1.0}, // COL_SKY
    {0.6, 0.6, 0.6, 1.0}, // COL_GREY
    {0.8, 1.0, 0.2, 1.0}, // COL_LIME
    {0.0, 0.5, 0.4, 1.0}  // COL_TEAL
};

// seleccion de color para la vista
#define VAL_UNKN COL_BLACK
#define VAL_VOID COL_RED
#define VAL_RESE COL_BLUE
#define VAL_PGTB COL_DARK_GREEN
#define VAL_COMP COL_YELLOW
#define VAL_KERN COL_MAGENTA
#define VAL_SLAB COL_TEAL

#define VAL_FILE COL_ORANGE
#define VAL_ANON COL_PURPLE
#define VAL_USER COL_BROWN
#define VAL_FREE COL_LIGHT_GREEN

#define VAL_ZONE_DMA COL_PINK
#define VAL_ZONE_DMA32 COL_GREY
#define VAL_ZONE_NORMAL COL_CYAN

#define VAL_WRITEBACK COL_SKY
#define VAL_DIRTY COL_LIME

#endif
