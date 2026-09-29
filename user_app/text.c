#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>  // snprintf
#include <stdint.h> // uint32_t
#include <string.h>

#include "../share.h"

#include "text.h"

uint32_t font_alpha[26] = {
    0x08A8FE31, // A
    0x3d1f463e, // B
    0x0C984126, // C
    0x3d18c63e, // D
    0x3f0f421f, // E
    0x3f0f4210, // F
    0x1D984E2F, // G
    0x231fc631, // H
    0x1c42108e, // I
    0x0e210a4c, // J
    0x232e4a31, // K
    0x2108421f, // L
    0x23bac631, // M
    0x239ace31, // N
    0x1d18c62e, // O
    0x3d1f4210, // P
    0x1d18de6f, // Q
    0x3d1f4a31, // R
    0x1f07043e, // S
    0x3e421084, // T
    0x2318c62e, // U
    0x2318c544, // V
    0x2318d771, // W
    0x22a22a31, // X
    0x22a21084, // Y
    0x3e11111f  // Z
};

uint32_t font_num[10] = {
    0x1d3ae62e, // 0
    0x08c2108e, // 1
    0x1d11111f, // 2
    0x1d11183e, // 3
    0x04657c42, // 4
    0x3f0f062e, // 5
    0x1d0f462e, // 6
    0x3e111084, // 7
    0x1d17462e, // 8
    0x1d17842e  // 9
};

void draw_text(const char* text, float start_x, float start_y, float size)
{
    // Compensar el estiramiento horizontal
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    float aspect = (float)viewport[2] / (float)(viewport[3] > 0 ? viewport[3] : 1);

    float size_x = size / aspect;
    float size_y = size;

    float x = start_x;
    glBegin(GL_QUADS);
    for (int i = 0; text[i] != '\0'; i++) {
        char c = text[i];
        if (c >= 'a' && c <= 'z') c -= 32;

        uint32_t bitmap = 0;
        if (c >= 'A' && c <= 'Z') bitmap = font_alpha[c - 'A'];
        else if (c >= '0' && c <= '9') bitmap = font_num[c - '0'];
        else if (c == ':') bitmap = 0x00401000;
        else if (c == '.') bitmap = 0x00000080;
        else if (c == '-') bitmap = 0x00070000;
        else if (c == '[') bitmap = 0x0c421086;
        else if (c == ']') bitmap = 0x1842108c;

        if (bitmap) {
            for (int row = 0; row < 6; row++) {
                for (int col = 0; col < 5; col++) {
                    if ((bitmap >> (29 - (row * 5 + col))) & 1) {
                        float px = x + col * size_x;
                        float py = start_y - row * size_y;
                        glVertex2f(px, py);
                        glVertex2f(px + size_x, py);
                        glVertex2f(px + size_x, py - size_y);
                        glVertex2f(px, py - size_y);
                    }
                }
            }
        }
        x += 6 * size_x;
    }
    glEnd();
}

#if !FORCE_WIN_TEXTURE

static const float HUD_PANEL_OUTER   = 0.98f;   // borde exterior de los paneles
static const float HUD_PANEL_INNER   = 0.60f;   // borde interior de los paneles
static const float HUD_PANEL_TOP     = 0.98f;
static const float HUD_PANEL_BOTTOM  = -0.98f;

static const float HUD_FONT_SIZE     = 0.006f;
static const float HUD_TITLE_SIZE    = 0.008f;
static const float HUD_ROW_STEP      = 0.05f;   // separación vertical entre filas

// Panel izquierdo
static const float HUD_LEFT_X        = -0.94f;
static const float HUD_LEGEND_Y      = 0.75f;   // primera fila de la leyenda
static const float HUD_TITLE_X       = -0.92f;
static const float HUD_TITLE_Y       = 0.90f;
static const float HUD_SUBTITLE_Y    = 0.80f;

// Panel derecho (tabla de contadores)
static const float HUD_AMOUNTS_X     = 0.63f;
static const float HUD_COUNTER_Y     = 0.80f;
static const float HUD_COUNTER_LABEL_X = 0.62f;
static const float HUD_COUNTER_VALUE_X = 0.76f;
static const float HUD_STATS_X       = 0.65f;

typedef enum {
    HUD_MODE_USE   = 0,
    HUD_MODE_ZONE  = 1,
    HUD_MODE_STATE = 2
} HudMode;

// Una fila de la leyenda (columna izquierda) y su contador (columna derecha)
typedef struct {
    const char *key;            // tecla del checkbox; NULL si el modo no tiene checkboxes
    const char *name;           // texto de la leyenda (con sangría)
    const char *counter_label;  // etiqueta del contador
    int         color;          // índice en palette
    uint32_t    mask;           // bit en la vista actual (solo modo USE)
    int         counter_index;  // posición del contador en map_ptr
    int         gap_before;     // 1 = dejar una fila vacía antes de esta
} HudEntry;

static const HudEntry HUD_USE_ENTRIES[] = {
    { "0", "  FREE",       "FREE:", VAL_FREE, MASK_FREE, INDEX_CONT_FREE, 0 },
    { "1", "  RESERVED",   "RESE:", VAL_RESE, MASK_RESE, INDEX_CONT_RESE, 0 },
    { "2", "  SLAB",       "SLAB:", VAL_SLAB, MASK_SLAB, INDEX_CONT_SLAB, 0 },
    { "3", "  HUGE",       "HUGE:", VAL_HUGE, MASK_HUGE, INDEX_CONT_HUGE, 0 },
    { "4", "  THP",        "THP :", VAL_THP,  MASK_THP,  INDEX_CONT_THP,  0 },
    { "5", "  COMPOUND",   "COMP:", VAL_COMP, MASK_COMP, INDEX_CONT_COMP, 0 },
    { "6", "  PGTB",       "PGTB:", VAL_PGTB, MASK_PGTB, INDEX_CONT_PGTB, 0 },
    { "7", "  ACTIVE",     "ACTI:", VAL_ACTI, MASK_ACTI, INDEX_CONT_ACTI, 0 },
    { "8", "  FILE",       "FILE:", VAL_FILE, MASK_FILE, INDEX_CONT_FILE, 0 },
    { "9", "  ANONYMOUS",  "ANON:", VAL_ANON, MASK_ANON, INDEX_CONT_ANON, 0 },
    { "U", "  USER",       "USER:", VAL_USER, MASK_USER, INDEX_CONT_USER, 1 },
    { "K", "  KERNEL",     "KERN:", VAL_KERN, MASK_KERN, INDEX_CONT_KERN, 0 },
};

static const HudEntry HUD_ZONE_ENTRIES[] = {
    { NULL, "  DMA",    "DMA  :", VAL_ZONE_DMA,    0, INDEX_CONT_DMA,    0 },
    { NULL, "  DMA32",  "DMA32:", VAL_ZONE_DMA32,  0, INDEX_CONT_DMA32,  0 },
    { NULL, "  NORMAL", "NORM :", VAL_ZONE_NORMAL, 0, INDEX_CONT_NORMAL, 0 },
};

static const HudEntry HUD_STATE_ENTRIES[] = {
    { NULL, "  WRITEBACK", "WRITE:", VAL_WRITEBACK, 0, INDEX_CONT_WRITEBACK, 0 },
    { NULL, "  DIRTY",     "DIRTY:", VAL_DIRTY,     0, INDEX_CONT_DIRTY,     0 },
};

static const char *const HUD_MODE_TITLES[] = {
    [HUD_MODE_USE]   = "VIEW USE",
    [HUD_MODE_ZONE]  = "VIEW ZONE",
    [HUD_MODE_STATE] = "VIEW STATE",
};

typedef struct { const char *text; float y; } HudHelpLine;

static const HudHelpLine HUD_HELP_LINES[] = {
    { "A:VIEW USE",    -0.40f },
    { "Z:VIEW ZONE",   -0.45f },
    { "S:VIEW STATE",  -0.50f },
    { "E:MORE ZOOM",   -0.60f },
    { "D:LESS ZOOM",   -0.65f },
    { "ARROW:MOVE",    -0.70f },
    { "R:RESET VIEW",  -0.75f },
};

#define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))


static void hud_set_text_color(void)
{
    glColor4f(0.9f, 0.9f, 0.9f, 1.0f);
}

static void hud_set_palette_color(int index)
{
    glColor4f(palette[index][0], palette[index][1], palette[index][2], palette[index][3]);
}

static uint64_t hud_read_counter(const uint8_t *map_ptr, int index)
{
    uint64_t value = 0;
    memcpy(&value, &map_ptr[index], sizeof(value));
    return value;
}

static HudMode hud_get_mode(const uint8_t *map_ptr)
{
    switch (map_ptr[INDEX_MODE]) {
        case 0:  return HUD_MODE_USE;
        case 1:  return HUD_MODE_ZONE;
        default: return HUD_MODE_STATE;
    }
}

static void hud_get_entries(HudMode mode, const HudEntry **entries, size_t *count)
{
    switch (mode) {
        case HUD_MODE_USE:
            *entries = HUD_USE_ENTRIES;   *count = ARRAY_LEN(HUD_USE_ENTRIES);   break;
        case HUD_MODE_ZONE:
            *entries = HUD_ZONE_ENTRIES;  *count = ARRAY_LEN(HUD_ZONE_ENTRIES);  break;
        default:
            *entries = HUD_STATE_ENTRIES; *count = ARRAY_LEN(HUD_STATE_ENTRIES); break;
    }
}

// Configura una proyección 2D normalizada para dibujar encima de la escena
static void hud_begin_2d(void)
{
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

static void hud_panel_vertices(float x_outer, float x_inner)
{
    glVertex2f(x_outer, HUD_PANEL_BOTTOM);
    glVertex2f(x_inner, HUD_PANEL_BOTTOM);
    glVertex2f(x_inner, HUD_PANEL_TOP);
    glVertex2f(x_outer, HUD_PANEL_TOP);
}

// Panel con fondo oscuro y borde blanco
static void hud_draw_panel(float x_outer, float x_inner)
{
    glColor4f(0.1f, 0.1f, 0.1f, 0.9f);
    glBegin(GL_QUADS);
    hud_panel_vertices(x_outer, x_inner);
    glEnd();

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    hud_panel_vertices(x_outer, x_inner);
    glEnd();
}

static void hud_draw_header(HudMode mode)
{
    hud_set_text_color();
    draw_text(HUD_MODE_TITLES[mode], HUD_TITLE_X, HUD_TITLE_Y, HUD_TITLE_SIZE);

    draw_text("AMOUNTS IN VIEW", HUD_AMOUNTS_X, HUD_TITLE_Y, HUD_FONT_SIZE);

    hud_set_palette_color(VAL_VOID);
    draw_text("  RESE BIOS", HUD_LEFT_X, HUD_SUBTITLE_Y, HUD_FONT_SIZE);
}

// Columna izquierda: [checkbox] + nombre de cada categoría, con su color
static void hud_draw_legend(const HudEntry *entries, size_t count, uint16_t current_view)
{
    char buffer[64];
    int row = 0;

    for (size_t i = 0; i < count; i++) {
        const HudEntry *e = &entries[i];
        row += e->gap_before;
        float y = HUD_LEGEND_Y - row * HUD_ROW_STEP;

        if (e->key) {
            hud_set_text_color();
            snprintf(buffer, sizeof(buffer), "%s:          [%s]",
                     e->key, (current_view & e->mask) ? "X" : " ");
            draw_text(buffer, HUD_LEFT_X, y, HUD_FONT_SIZE);
        }

        hud_set_palette_color(e->color);
        draw_text(e->name, HUD_LEFT_X, y, HUD_FONT_SIZE);
        row++;
    }
}

// Columna derecha: tabla "ETIQUETA: valor"
static void hud_draw_counters(const uint8_t *map_ptr, const HudEntry *entries, size_t count)
{
    char buffer[64];
    int row = 0;

    for (size_t i = 0; i < count; i++) {
        const HudEntry *e = &entries[i];
        row += e->gap_before;
        float y = HUD_COUNTER_Y - row * HUD_ROW_STEP;

        hud_set_palette_color(e->color);
        draw_text(e->counter_label, HUD_COUNTER_LABEL_X, y, HUD_FONT_SIZE);

        hud_set_text_color();
        snprintf(buffer, sizeof(buffer), "%lu",
                 (unsigned long)hud_read_counter(map_ptr, e->counter_index));
        draw_text(buffer, HUD_COUNTER_VALUE_X, y, HUD_FONT_SIZE);
        row++;
    }
}

// Ayuda de selección: solo tiene sentido en el modo USE
static void hud_draw_selection_help(void)
{
    hud_set_text_color();
    draw_text("A:SELECT ALL", HUD_LEFT_X, 0.05f, HUD_FONT_SIZE);
    draw_text("X:INV SELECT", HUD_LEFT_X, 0.00f, HUD_FONT_SIZE);
}

// Atajos de teclado en la parte inferior izquierda
static void hud_draw_key_help(void)
{
    hud_set_text_color();

    for (size_t i = 0; i < ARRAY_LEN(HUD_HELP_LINES); i++)
        draw_text(HUD_HELP_LINES[i].text, HUD_LEFT_X, HUD_HELP_LINES[i].y, HUD_FONT_SIZE);

    if (CAPT_VIDEO == 0)
        draw_text("F:FULL SCREEN", HUD_LEFT_X, -0.85f, HUD_FONT_SIZE);
    draw_text("Q:QUIT", HUD_LEFT_X, -0.90f, HUD_FONT_SIZE);
}

// KUPS, FPS y estado de grabación (parte inferior derecha)
static void hud_draw_stats(uint8_t kups, double fps)
{
    char buffer[64];

    hud_set_text_color();

    snprintf(buffer, sizeof(buffer), "KUPS: %d", kups);
    draw_text(buffer, HUD_STATS_X, -0.75f, HUD_FONT_SIZE);

    snprintf(buffer, sizeof(buffer), "FPS: %.1f", fps);
    draw_text(buffer, HUD_STATS_X, -0.80f, HUD_FONT_SIZE);

#if CAPT_VIDEO
    snprintf(buffer, sizeof(buffer), "REC [%d FPS]", TARGET_FPS);
    draw_text(buffer, HUD_STATS_X, -0.90f, HUD_FONT_SIZE);
#else
    draw_text("NO REC", HUD_STATS_X, -0.90f, HUD_FONT_SIZE);
#endif
}

#endif // Fin !FORCE_WIN_TEXTURE


void show_hud(uint8_t *map_ptr, double fps)
{
#if FORCE_WIN_TEXTURE
    (void)map_ptr;
    (void)fps;
#else
    const HudMode mode = hud_get_mode(map_ptr);
    const HudEntry *entries;
    size_t count;
    hud_get_entries(mode, &entries, &count);

    hud_begin_2d();

    hud_draw_panel(-HUD_PANEL_OUTER, -HUD_PANEL_INNER);  // izquierdo
    hud_draw_panel( HUD_PANEL_OUTER,  HUD_PANEL_INNER);  // derecho

    hud_draw_header(mode);

    uint16_t current_view = 0;
    if (mode == HUD_MODE_USE)
        memcpy(&current_view, &map_ptr[INDEX_VIEW], sizeof(current_view));

    hud_draw_legend(entries, count, current_view);
    if (mode == HUD_MODE_USE)
        hud_draw_selection_help();
    hud_draw_counters(map_ptr, entries, count);

    hud_draw_key_help();
    hud_draw_stats(map_ptr[INDEX_KUPS], fps);
#endif
}
