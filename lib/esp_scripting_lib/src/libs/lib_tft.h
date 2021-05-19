
#pragma once
#ifndef LIB_TFT_ESP32_H
#define LIB_TFT_ESP32_H

#include "scr_lib.h"

#include <TFT_eSPI.h>

#ifdef SCR_CONFIG_TFT_LIB_ENABLE_JPEG_MODULE
    #include <JPEGDecoder.h>
#endif

static TFT_eSPI _tft; // TFT_eSPI

static bool is_initialized = false;

/************************/

#ifdef SCR_CONFIG_TFT_LIB_ENABLE_JPEG_MODULE

static void tftJpegRender(int xpos, int ypos) {
    uint16_t *pImg;
    uint16_t mcu_w = JpegDec.MCUWidth;
    uint16_t mcu_h = JpegDec.MCUHeight;
    uint32_t max_x = JpegDec.width;
    uint32_t max_y = JpegDec.height;

    bool swapBytes = _tft.getSwapBytes();
    _tft.setSwapBytes(true);

    uint32_t min_w = jpg_min(mcu_w, max_x % mcu_w);
    uint32_t min_h = jpg_min(mcu_h, max_y % mcu_h);

    // save the current image block size
    uint32_t win_w = mcu_w;
    uint32_t win_h = mcu_h;

    // save the coordinate of the right and bottom edges to assist image cropping
    // to the screen size
    max_x += xpos;
    max_y += ypos;

    // Fetch data from the file, decode and display
    while (JpegDec.read()) {    // While there is more data in the file
        pImg = JpegDec.pImage ;   // Decode a MCU (Minimum Coding Unit, typically a 8x8 or 16x16 pixel block)

        // Calculate coordinates of top left corner of current MCU
        int mcu_x = JpegDec.MCUx * mcu_w + xpos;
        int mcu_y = JpegDec.MCUy * mcu_h + ypos;

        // check if the image block size needs to be changed for the right edge
        if (mcu_x + mcu_w <= max_x) win_w = mcu_w;
        else win_w = min_w;

        // check if the image block size needs to be changed for the bottom edge
        if (mcu_y + mcu_h <= max_y) win_h = mcu_h;
        else win_h = min_h;

        // copy pixels into a contiguous block
        if (win_w != mcu_w)
        {
            uint16_t *cImg;
            int p = 0;
            cImg = pImg + win_w;
            for (int h = 1; h < win_h; h++)
            {
                p += mcu_w;
                for (int w = 0; w < win_w; w++)
                {
                *cImg = *(pImg + w + p);
                cImg++;
                }
            }
        }

        // draw image MCU block only if it will fit on the screen
        if (( mcu_x + win_w ) <= _tft.width() && ( mcu_y + win_h ) <= _tft.height())
        _tft.pushImage(mcu_x, mcu_y, win_w, win_h, pImg);
        else if ( (mcu_y + win_h) >= _tft.height())
        JpegDec.abort(); // Image has run off bottom of screen so abort decoding
    }

    _tft.setSwapBytes(swapBytes);
}

static void tftDrawSdJpeg(const char *filename, int xpos, int ypos) {

  // Open the named file (the Jpeg decoder library will close it)
  File jpegFile = SD.open( filename, FILE_READ);  // or, file handle reference for SD library
 
  if ( !jpegFile ) {
    Serial.print("ERROR: File \""); Serial.print(filename); Serial.println ("\" not found!");
    return;
  }

  // Use one of the following methods to initialise the decoder:
  boolean decoded = JpegDec.decodeSdFile(jpegFile);  // Pass the SD file handle to the decoder,
  if (decoded) {
    tftJpegRender(xpos, ypos);
  }
  else {
    Serial.println("Jpeg file format not supported!");
  }
}

#endif // SCR_CONFIG_TFT_LIB_ENABLE_JPEG_MODULE

/************************/


static int TFT_init(stack_t *&sp) {
    uint32_t rotation = POP();
    if (!is_initialized){
        _tft.init();
        is_initialized = true;
    }
    _tft.setRotation(constrain(rotation, 0, 3));
    _tft.setCursor(0, 0);
    _tft.setTextColor(0xFFFF);
    _tft.setTextWrap(true);
    return 1;
}

static int TFT_fill_screen(stack_t *&sp) {
    uint32_t color = POP();
    _tft.fillScreen(color);
    return 1;
}

static int TFT_write(stack_t *&sp) {
    uint32_t v1 = POP();
    char *str = (char*)(heap_ref()->getAddr(v1));
    _tft.print(str);
    return 1;
}

static int TFT_draw_pixel(stack_t *&sp) {
    uint32_t color = POP();
    uint32_t height = POP();
    uint32_t width = POP();
    _tft.drawPixel(width, height, color);
    return 1;
}

static int TFT_width(stack_t *&sp) {
    PUSH(_tft.width());
    return 1;
}

static int TFT_height(stack_t *&sp) {
    PUSH(_tft.height());
    return 1;
}

static int TFT_set_cursor(stack_t *&sp) {
    uint32_t y = POP();
    uint32_t x = POP();
    _tft.setCursor(x, y);
    return 1;
}

static int TFT_get_cursor_x(stack_t *&sp) {
    PUSH(_tft.getCursorX());
    return 1;
}

static int TFT_get_cursor_y(stack_t *&sp) {
    PUSH(_tft.getCursorY());
    return 1;
}

static int TFT_set_text_color(stack_t *&sp) {
    uint32_t color = POP();
    _tft.setTextColor(color);
    return 1;
}

static int TFT_draw_line(stack_t *&sp) {
    uint32_t color = POP();
    uint32_t ye = POP();
    uint32_t xe = POP();
    uint32_t ys = POP();
    uint32_t xs = POP();
    _tft.drawLine(xs, ys, xe, ye, color);
    return 1;
}

static int TFT_fill_rect(stack_t *&sp) {
    uint32_t color = POP();
    uint32_t h = POP();
    uint32_t w = POP();
    uint32_t y = POP();
    uint32_t x = POP();
    _tft.fillRect(x, y, w, h, color);
    return 1;
}

static int TFT_rgb_to_565(stack_t *&sp) {
    uint32_t b = POP();
    uint32_t g = POP();
    uint32_t r = POP();
    uint32_t code = (r & 0x1f) << 11;
    code |= (g & 0x3f) << 5;
    code |= (b & 0x1f) << 0;
    PUSH(code);
    return 1;
}

static int TFT_set_text_font(stack_t *&sp) {
    uint32_t font = POP();
    font = constrain(font, 1, 20);
    if (font <= 9) {
        _tft.setTextFont(font);
    } else {
#ifdef LOAD_GFXFF
        switch (font) {
            case 10: _tft.setFreeFont(&FreeMono9pt7b); break;
            case 11: _tft.setFreeFont(&FreeMono12pt7b); break;
            case 12: _tft.setFreeFont(&FreeSerif9pt7b); break;
            case 13: _tft.setFreeFont(&FreeSerif12pt7b); break;
            case 14: _tft.setFreeFont(&FreeSerifBold12pt7b); break;
            case 15: _tft.setFreeFont(&FreeSerifItalic12pt7b); break;
            case 16: _tft.setFreeFont(&FreeSerifBoldItalic12pt7b); break;
            case 17: _tft.setFreeFont(&FreeSans12pt7b); break;
            case 18: _tft.setFreeFont(&FreeSans18pt7b); break;
            case 19: _tft.setFreeFont(&FreeSans24pt7b); break;
            case 20: _tft.setFreeFont(&FreeSerifBoldItalic12pt7b); break;
            default:
                _tft.setTextFont(1);
            break;
        }
#endif
    }
    return 1;
}

#ifdef SCR_CONFIG_TFT_LIB_ENABLE_JPEG_MODULE

static int TFT_draw_sd_jpeg(stack_t *&sp) {
    int y = stack_to_int(POP());
    int x = stack_to_int(POP());
    uint32_t v1 = POP();
    char *filename = (char*)(heap_ref()->getAddr(v1));
    bool isEnabled = isSDEnabled();
    if (!isEnabled) {
        isEnabled = SD.begin();
    }
    if (isEnabled) {
        tftDrawSdJpeg(filename, x, y);
        // Если ФС уже была открыта, то закрывать её нельзя
        if (!isSDEnabled())
            SD.end();
    }
    return 1;
}

#endif // SCR_CONFIG_TFT_LIB_ENABLE_JPEG_MODULE

static int TFT_draw_image(stack_t *&sp) {
    int y = stack_to_int(POP());
    int x = stack_to_int(POP());
    uint32_t v1 = POP();
    heaphdr_t *header = heap_ref()->getHeader(v1);
    uint32_t block_length = header->len;
    uint8_t *block = (uint8_t*)(header + 1);
    uint32_t width = block[0], height = block[1];
    if (block_length < width * height + 2)
        return 1;
    _tft.pushImage(x, y, width, height, block + 2);
    return 1;
}

const struct lib_reg lib_tft32[] = {
    /* tft.init(int rotation) */
    {"init", TFT_init, 2, new VarType[2] {
        VarType::None, VarType::Int
    }},
    /* tft.fill_screen(int color) */
    {"fill_screen", TFT_fill_screen, 2, new VarType[2] {
        VarType::None, VarType::Int
    }},
    /* tft.write(string text) */
    {"write", TFT_write, 2, new VarType[2] {
        VarType::None, VarType::String
    }},
    /* tft.draw_pixel(int width, int height, int color) */
    {"draw_pixel", TFT_draw_pixel, 4, new VarType[4] {
        VarType::None, VarType::Int, VarType::Int, VarType::Int
    }},
    /* int tft.width() */
    {"width", TFT_width, 1, new VarType[1] {
        VarType::Int
    }},
    /* int tft.height() */
    {"height", TFT_height, 1, new VarType[1] {
        VarType::Int
    }},
    /* tft.set_cursor(int x, int y) */
    {"set_cursor", TFT_set_cursor, 3, new VarType[3] {
        VarType::None, VarType::Int, VarType::Int
    }},
    /* int tft.get_cursor_x() */
    {"get_cursor_x", TFT_get_cursor_x, 1, new VarType[1] {
        VarType::Int
    }},
    /* int tft.get_cursor_y() */
    {"get_cursor_y", TFT_get_cursor_y, 1, new VarType[1] {
        VarType::Int
    }},
    /* tft.set_text_color(int color) */
    {"set_text_color", TFT_set_text_color, 2, new VarType[2] {
        VarType::None, VarType::Int
    }},
    /* tft.draw_line(int xs, int ys, int xe, int ye, int color) */
    {"draw_line", TFT_draw_line, 6, new VarType[6] {
        VarType::None, VarType::Int, VarType::Int, VarType::Int, VarType::Int, VarType::Int
    }},
    /* tft.fill_rect(int x, int y, int w, int h, int color) */
    {"fill_rect", TFT_fill_rect, 6, new VarType[6] {
        VarType::None, VarType::Int, VarType::Int, VarType::Int, VarType::Int, VarType::Int
    }},
    /* int tft.rgb_to_565(int r, int g, int b) */
    {"rgb", TFT_rgb_to_565, 4, new VarType[4] {
        VarType::Int, VarType::Int, VarType::Int, VarType::Int
    }},
    /* tft.set_text_font(int font) */
    {"set_font", TFT_set_text_font, 2, new VarType[2] {
        VarType::None, VarType::Int
    }},
    /* tft.set_cursor(int_array image, int x, int y) */
    {"draw_image", TFT_draw_image, 4, new VarType[4] {
        VarType::None, VarType::String, VarType::Int, VarType::Int
    }},
#ifdef SCR_CONFIG_TFT_LIB_ENABLE_JPEG_MODULE
    /* tft.draw_sd_jpeg(string filename, int x, int y) */
    {"draw_sd_jpeg", TFT_draw_sd_jpeg, 4, new VarType[4] {
        VarType::None, VarType::String, VarType::Int, VarType::Int
    }}
#endif
};

#endif // LIB_TFT_ESP32_H