
#pragma once
#ifndef LIB_ESP_VGA_H
#define LIB_ESP_VGA_H

#include "scr_lib.h"

#include <VGA/VGA6Bit.h>
#include <Ressources/CodePage437_9x16.h>

static bool vgaIsInitialized = false;

static const PinConfig pinConf(-1, -1, -1, 14, 12,  -1, -1, -1, 27, 33,  -1, -1, 32, 22,  16, 17,  -1);

static VGA6Bit vga;

static void freeBuffer(unsigned char **buffer) {
    for (int y = 0; y < vga.yres; y++)
    {
        free(buffer[y]);
    }
}

static void deleteBuffers() {
    for (int i = 0; i < vga.frameBufferCount; i++) {
		if (vga.frameBuffers[i]) {
            freeBuffer(vga.frameBuffers[i]);
            vga.frameBuffers[i] = nullptr;
        }
    }
    vga.frameBufferCount = 1;
}

static int LIB_VGA_init(stack_t *&sp) {
    uint32_t isDoubleBuffered = POP();
    deleteBuffers();
    if (!vgaIsInitialized) {
        if (isDoubleBuffered != 0) {
            vga.setFrameBufferCount(2);
            Mode vgaMode = vga.MODE320x200;//vga.MODE640x480.custom(320, 200);
            vgaIsInitialized = vga.init(vgaMode, pinConf);
        } else {
            vgaIsInitialized = vga.init(vga.MODE400x300, pinConf);
        }
    }
    vga.setFont(CodePage437_9x16);
    return 1;
}

static int LIB_VGA_end(stack_t *&sp) {
    if (vgaIsInitialized) {
        deleteBuffers();
        vgaIsInitialized = false;
    }
    return 1;
}

static int LIB_VGA_draw_buffer(stack_t *&sp) {
    vga.show();
    return 1;
}

static int LIB_VGA_fill_screen(stack_t *&sp) {
    int32_t color = stack_to_int(POP());
    vga.clear(color);
    return 1;
}

static int LIB_VGA_write(stack_t *&sp) {
    uint32_t v1 = POP();
    char *str = (char*)(heap_ref()->getAddr(v1));
    if (vgaIsInitialized) {
        vga.print(str);
    }
    return 1;
}

static int LIB_VGA_set_text_color(stack_t *&sp) {
    int32_t color = stack_to_int(POP());
    vga.setTextColor(color);
    return 1;
}

static int LIB_VGA_width(stack_t *&sp) {
    PUSH(vga.xres);
    return 1;
}

static int LIB_VGA_height(stack_t *&sp) {
    PUSH(vga.yres);
    return 1;
}

static int LIB_VGA_get(stack_t *&sp) {
    uint32_t y = POP();
    uint32_t x = POP();
    PUSH(vga.get(x, y));
    return 1;
}

static int LIB_VGA_set_cursor(stack_t *&sp) {
    uint32_t y = POP();
    uint32_t x = POP();
    vga.setCursor(x, y);
    return 1;
}

static int LIB_VGA_rgb(stack_t *&sp) {
    uint32_t b = POP();
    uint32_t g = POP();
    uint32_t r = POP();
    uint32_t code = vga.RGB(r, g, b);
    PUSH(code);
    return 1;
}

static int LIB_VGA_draw_pixel(stack_t *&sp) {
    uint32_t color = POP();
    uint32_t y = POP();
    uint32_t x = POP();
    if (vgaIsInitialized) {
        vga.dot(x, y, color);
    }
    return 1;
}

static int LIB_VGA_draw_line(stack_t *&sp) {
    uint32_t color = POP();
    uint32_t y2 = stack_to_int(POP());
    uint32_t x2 = stack_to_int(POP());
    uint32_t y1 = stack_to_int(POP());
    uint32_t x1 = stack_to_int(POP());
    if (!vgaIsInitialized)
        return 1;
    vga.line(x1, y1, x2, y2, color);
    return 1;
}

static int LIB_VGA_draw_hline(stack_t *&sp) {
    uint32_t color = POP();
    uint32_t w = POP();
    uint32_t y = stack_to_int(POP());
    uint32_t x = stack_to_int(POP());
    if (vgaIsInitialized) {
        vga.xLine(x, x + w, y, color);
    }
    return 1;
}

static int LIB_VGA_draw_rect(stack_t *&sp) {
    uint32_t color = POP();
    uint32_t height = POP();
    uint32_t width = POP();
    uint32_t y = stack_to_int(POP());
    uint32_t x = stack_to_int(POP());
    if (vgaIsInitialized) {
        vga.rect(x, y, width, height, color);
    }
    return 1;
}

static int LIB_VGA_fill_rect(stack_t *&sp) {
    uint32_t color = POP();
    uint32_t height = POP();
    uint32_t width = POP();
    uint32_t y = stack_to_int(POP());
    uint32_t x = stack_to_int(POP());
    if (vgaIsInitialized) {
        vga.fillRect(x, y, width, height, color);
    }
    return 1;
}

static int LIB_VGA_draw_circle(stack_t *&sp) {
    uint32_t color = POP();
    uint32_t r = POP();
    uint32_t y = stack_to_int(POP());
    uint32_t x = stack_to_int(POP());
    if (vgaIsInitialized) {
        vga.circle(x, y, r, color);
    }
    return 1;
}

static int LIB_VGA_fill_circle(stack_t *&sp) {
    uint32_t color = POP();
    uint32_t r = POP();
    uint32_t y = stack_to_int(POP());
    uint32_t x = stack_to_int(POP());
    if (vgaIsInitialized) {
        vga.fillCircle(x, y, r, color);
    }
    return 1;
}

static int LIB_VGA_draw_triangle(stack_t *&sp) {
    uint32_t color = POP();
    uint32_t y3 = stack_to_int(POP());
    uint32_t x3 = stack_to_int(POP());
    uint32_t y2 = stack_to_int(POP());
    uint32_t x2 = stack_to_int(POP());
    uint32_t y1 = stack_to_int(POP());
    uint32_t x1 = stack_to_int(POP());
    short v1[]{(short)x1, (short)y1};
    short v2[]{(short)x2, (short)y2};
    short v3[]{(short)x3, (short)y3};
    if (vgaIsInitialized) {
        vga.triangle(v1, v2, v3, color);
    }
    return 1;
}

const struct lib_reg lib_esp_vga[] = {
    /* vga.init(int is_buffered) */
    {"init", LIB_VGA_init, 2, new VarType::Std[2] {
        VarType::Std::None, VarType::Std::Int
    }},
    /* vga.fill_screen(int color) */
    {"fill_screen", LIB_VGA_fill_screen, 2, new VarType::Std[2] {
        VarType::Std::None, VarType::Std::Int
    }},
    /* vga.write(string s) */
    {"write", LIB_VGA_write, 2, new VarType::Std[2] {
        VarType::Std::None, VarType::Std::String
    }},
    /* vga.write(string s) */
    {"set_text_color", LIB_VGA_set_text_color, 2, new VarType::Std[2] {
        VarType::Std::None, VarType::Std::Int
    }},
    /* int vga.rgb(int r, int g, int b) */
    {"rgb", LIB_VGA_rgb, 4, new VarType::Std[4] {
        VarType::Std::Int, VarType::Std::Int, VarType::Std::Int, VarType::Std::Int
    }},
    /* vga.draw_pixel(int x, int y, int color) */
    {"draw_pixel", LIB_VGA_draw_pixel, 4, new VarType::Std[4] {
        VarType::Std::None, VarType::Std::Int, VarType::Std::Int, VarType::Std::Int
    }},
    /* vga.draw_line(int x1, int y1, int x2, int y2, int color) */
    {"draw_line", LIB_VGA_draw_line, 6, new VarType::Std[6] {
        VarType::Std::None, VarType::Std::Int, VarType::Std::Int, VarType::Std::Int, VarType::Std::Int, VarType::Std::Int
    }},
    /* vga.draw_hline(int x, int y, int width, int color) */
    {"draw_hline", LIB_VGA_draw_hline, 5, new VarType::Std[5] {
        VarType::Std::None, VarType::Std::Int, VarType::Std::Int, VarType::Std::Int, VarType::Std::Int
    }},
    /* vga.draw_rect(int x, int y, int width, int height, int color) */
    {"draw_rect", LIB_VGA_draw_rect, 6, new VarType::Std[6] {
        VarType::Std::None, VarType::Std::Int, VarType::Std::Int, VarType::Std::Int, VarType::Std::Int, VarType::Std::Int
    }},
    /* vga.fill_rect(int x, int y, int width, int height, int color) */
    {"fill_rect", LIB_VGA_fill_rect, 6, new VarType::Std[6] {
        VarType::Std::None, VarType::Std::Int, VarType::Std::Int, VarType::Std::Int, VarType::Std::Int, VarType::Std::Int
    }},
    /* vga.draw_circle(int x, int y, int r, int color) */
    {"draw_circle", LIB_VGA_draw_circle, 5, new VarType::Std[5] {
        VarType::Std::None, VarType::Std::Int, VarType::Std::Int, VarType::Std::Int, VarType::Std::Int
    }},
    /* vga.fill_circle(int x, int y, int r, int color) */
    {"fill_circle", LIB_VGA_fill_circle, 5, new VarType::Std[5] {
        VarType::Std::None, VarType::Std::Int, VarType::Std::Int, VarType::Std::Int, VarType::Std::Int
    }},
    /* vga.draw_triangle(int x1, int y1, int x2, int y2, int x3, int y3, int color) */
    {"draw_triangle", LIB_VGA_draw_triangle, 8, new VarType::Std[8] {
        VarType::Std::None, VarType::Std::Int, VarType::Std::Int, VarType::Std::Int, VarType::Std::Int, VarType::Std::Int, VarType::Std::Int, VarType::Std::Int
    }},
    /* int vga.width() */
    {"width", LIB_VGA_width, 1, new VarType::Std[1] {
        VarType::Std::Int
    }},
    /* int vga.height() */
    {"height", LIB_VGA_height, 1, new VarType::Std[1] {
        VarType::Std::Int
    }},
    /* int vga.get(int x, int y) */
    {"get", LIB_VGA_get, 3, new VarType::Std[3] {
        VarType::Std::Int, VarType::Std::Int, VarType::Std::Int
    }},
    /* vga.set_cursor(int x, int y) */
    {"set_cursor", LIB_VGA_set_cursor, 3, new VarType::Std[3] {
        VarType::Std::None, VarType::Std::Int, VarType::Std::Int
    }},
    /* vga.draw_buffer() */
    {"draw_buffer", LIB_VGA_draw_buffer, 1, new VarType::Std[1] {
        VarType::Std::None
    }},
    /* vga.delete_buffer() */
    {"delete_buffer", LIB_VGA_end, 1, new VarType::Std[1] {
        VarType::Std::None
    }}
};

#endif