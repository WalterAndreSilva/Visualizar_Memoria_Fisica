#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>  // snprintf
#include <stdint.h> // uint32_t
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

void show_hud(uint8_t *map_ptr)
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

    // Fondo semitransparente
    // Rango X: -0.98 hasta -0.6
    glColor4f(0.1f, 0.1f, 0.1f, 0.9f);
    glBegin(GL_QUADS);
    glVertex2f(-0.98f, -0.98f);
    glVertex2f(-0.60f, -0.98f);
    glVertex2f(-0.60f,  0.98f);
    glVertex2f(-0.98f,  0.98f);
    glEnd();

    // Borde
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(-0.98f, -0.98f);
    glVertex2f(-0.60f, -0.98f);
    glVertex2f(-0.60f,  0.98f);
    glVertex2f(-0.98f,  0.98f);
    glEnd();

    // Textos
    glColor4f(0.9f, 0.9f, 0.9f, 1.0f); // Blanco
    char *mode;
    if (map_ptr[INDEX_MODE]==0) mode ="VIEW USE";
    else if (map_ptr[INDEX_MODE]==1) mode = "VIEW ZONE";
    else mode = "VIEW STATE";
    draw_text(mode, -0.92f, 0.9f, 0.008f);

    char buffer[256];
    glColor4f(0.9f, 0.9f, 0.9f, 1.0f); // Blanco

    float font_size = 0.006f;

    // BLANCO
    snprintf(buffer, sizeof(buffer), "1:          [%s]", (map_ptr[INDEX_VIEW]& MASK_FREE) ? "X" : " ");
    draw_text(buffer, -0.94f, 0.8f, font_size);
    snprintf(buffer, sizeof(buffer), "2:          [%s]", (map_ptr[INDEX_VIEW]& MASK_PGTB) ? "X" : " ");
    draw_text(buffer, -0.94f, 0.7f, font_size);
    snprintf(buffer, sizeof(buffer), "3:          [%s]", (map_ptr[INDEX_VIEW]& MASK_RESE) ? "X" : " ");
    draw_text(buffer, -0.94f, 0.6f, font_size);
    snprintf(buffer, sizeof(buffer), "4:          [%s]", (map_ptr[INDEX_VIEW]& MASK_COMP) ? "X" : " ");
    draw_text(buffer, -0.94f, 0.5f, font_size);
    snprintf(buffer, sizeof(buffer), "5:          [%s]", (map_ptr[INDEX_VIEW]& MASK_FILE) ? "X" : " ");
    draw_text(buffer, -0.94f, 0.4f, font_size);
    snprintf(buffer, sizeof(buffer), "6:          [%s]", (map_ptr[INDEX_VIEW]& MASK_ANON) ? "X" : " ");
    draw_text(buffer, -0.94f, 0.3f, font_size);
    snprintf(buffer, sizeof(buffer), "7:          [%s]", (map_ptr[INDEX_VIEW]& MASK_USER) ? "X" : " ");
    draw_text(buffer, -0.94f, 0.2f, font_size);
    snprintf(buffer, sizeof(buffer), "8:          [%s]", (map_ptr[INDEX_VIEW]& MASK_KERN) ? "X" : " ");
    draw_text(buffer, -0.94f, 0.1f, font_size);

    // COLOR
    glColor4f(palette[VAL_FREE][0], palette[VAL_FREE][1], palette[VAL_FREE][2], palette[VAL_FREE][3]);
    draw_text("  FREE", -0.94f, 0.8f, font_size);
    glColor4f(palette[VAL_PGTB][0], palette[VAL_PGTB][1], palette[VAL_PGTB][2], palette[VAL_PGTB][3]);
    draw_text("  PGTB", -0.94f, 0.7f, font_size);
    glColor4f(palette[VAL_RESE][0], palette[VAL_RESE][1], palette[VAL_RESE][2], palette[VAL_RESE][3]);
    draw_text("  RESERVED", -0.94f, 0.6f, font_size);
    glColor4f(palette[VAL_COMP][0], palette[VAL_COMP][1], palette[VAL_COMP][2], palette[VAL_COMP][3]);
    draw_text("  COMPOUND", -0.94f, 0.5f, font_size);
    glColor4f(palette[VAL_FILE][0], palette[VAL_FILE][1], palette[VAL_FILE][2], palette[VAL_FILE][3]);
    draw_text("  FILE", -0.94f, 0.4f, font_size);
    glColor4f(palette[VAL_ANON][0], palette[VAL_ANON][1], palette[VAL_ANON][2], palette[VAL_ANON][3]);
    draw_text("  ANONYMOUS", -0.94f, 0.3f, font_size);
    glColor4f(palette[VAL_USER][0], palette[VAL_USER][1], palette[VAL_USER][2], palette[VAL_USER][3]);
    draw_text("  USER", -0.94f, 0.2f, font_size);
    glColor4f(palette[VAL_KERN][0], palette[VAL_KERN][1], palette[VAL_KERN][2], palette[VAL_KERN][3]);
    draw_text("  KERNEL", -0.94f, 0.1f, font_size);

    glColor4f(0.9f, 0.9f, 0.9f, 1.0f); //BLANCO

    draw_text("A:SELECT ALL", -0.94f, -0.1f, font_size);
    draw_text("X:INV SELECT", -0.94f, -0.2f, font_size);
    draw_text("Z:VIEW ZONE", -0.94f, -0.3f, font_size);

    glColor4f(palette[VAL_ZONE_DMA][0], palette[VAL_ZONE_DMA][1], palette[VAL_ZONE_DMA][2], palette[VAL_ZONE_DMA][3]);
    draw_text("  DMA", -0.94f, -0.4f, font_size);
    glColor4f(palette[VAL_ZONE_DMA32][0], palette[VAL_ZONE_DMA32][1], palette[VAL_ZONE_DMA32][2], palette[VAL_ZONE_DMA32][3]);
    draw_text("  DMA32", -0.94f, -0.5f, font_size);
    glColor4f(palette[VAL_ZONE_NORMAL][0], palette[VAL_ZONE_NORMAL][1], palette[VAL_ZONE_NORMAL][2], palette[VAL_ZONE_NORMAL][3]);
    draw_text("  NORMAL", -0.94f, -0.6f, font_size);

    glColor4f(0.9f, 0.9f, 0.9f, 1.0f); //BLANCO

    draw_text("S:VIEW STATE", -0.94f, -0.7f, font_size);

    glColor4f(palette[VAL_WRITEBACK][0], palette[VAL_WRITEBACK][1], palette[VAL_WRITEBACK][2], palette[VAL_WRITEBACK][3]);
    draw_text("  WRITEBACK", -0.94f, -0.8f, font_size);
    glColor4f(palette[VAL_DIRTY][0], palette[VAL_DIRTY][1], palette[VAL_DIRTY][2], palette[VAL_DIRTY][3]);
    draw_text("  DIRTY", -0.94f, -0.9f, font_size);
}
