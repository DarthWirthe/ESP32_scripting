
#pragma once
#ifndef LIB_TFT_ESP32_H
#define LIB_TFT_ESP32_H

#include "scr_lib.h"

#include <TFT_eSPI.h>

#ifdef SCR_CONFIG_TFT_LIB_ENABLE_JPEG_MODULE
    #include <JPEGDecoder.h>
#endif

static TFT_eSPI _tft; // TFT_eSPI

static TFT_eSprite _tftdb = TFT_eSprite(&_tft);

static bool is_initialized = false;
static bool isBuffered = false;

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
    _tft.fillScreen(0);
    _tft.setCursor(0, 0);
    _tft.setTextWrap(true);
    return 1;
}

static int TFT_init_buffered(stack_t *&sp) {
    uint32_t rotation = POP();
    if (!is_initialized){
        _tft.init();
        is_initialized = true;
    }
    _tft.setRotation(constrain(rotation, 0, 3));
    _tftdb.deleteSprite();
    _tftdb.createSprite(_tft.width(), _tft.height());
    _tftdb.fillSprite(0);
    _tftdb.setCursor(0, 0);
    _tftdb.setTextWrap(true);
    isBuffered = true;
    return 1;
}

static int TFT_end(stack_t *&sp) {
    if (isBuffered) {
        _tftdb.deleteSprite();
        isBuffered = false;
    }
    return 1;
}

static int TFT_fill_screen(stack_t *&sp) {
    uint32_t color = POP();
    if (isBuffered)
        _tftdb.fillSprite(color);
    else
        _tft.fillScreen(color);
    return 1;
}

static int TFT_write(stack_t *&sp) {
    uint32_t v1 = POP();
    char *str = (char*)(heap_ref()->getAddr(v1));
    if (isBuffered)
        _tftdb.print(str);
    else
        _tft.print(str);
    return 1;
}

static int TFT_draw_string(stack_t *&sp) {
    uint32_t font = POP();
    uint32_t y = stack_to_int(POP());
    uint32_t x = stack_to_int(POP());
    uint32_t pt = POP();
    char *str = (char*)(heap_ref()->getAddr(pt));
    if (isBuffered)
        _tftdb.drawString(str, x, y, font);
    else
        _tft.drawString(str, x, y, font);
    return 1;
}

static int TFT_draw_centre_string(stack_t *&sp) {
    uint32_t font = POP();
    uint32_t y = stack_to_int(POP());
    uint32_t x = stack_to_int(POP());
    uint32_t pt = POP();
    char *str = (char*)(heap_ref()->getAddr(pt));
    if (isBuffered)
        _tftdb.drawCentreString(str, x, y, font);
    else
        _tft.drawCentreString(str, x, y, font);
    return 1;
}

static int TFT_draw_pixel(stack_t *&sp) {
    uint32_t color = POP();
    uint32_t y = POP();
    uint32_t x = POP();
    if (isBuffered)
        _tftdb.drawPixel(x, y, color);
    else
        _tft.drawPixel(x, y, color);
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
    uint32_t y = stack_to_int(POP());
    uint32_t x = stack_to_int(POP());
    if (isBuffered)
        _tftdb.setCursor(x, y);
    else
        _tft.setCursor(x, y);
    return 1;
}

static int TFT_get_cursor_x(stack_t *&sp) {
    int16_t x;
    if (isBuffered)
        x = _tftdb.getCursorX();
    else
        x = _tft.getCursorX();
    PUSH(x);
    return 1;
}

static int TFT_get_cursor_y(stack_t *&sp) {
    int16_t y;
    if (isBuffered)
        y = _tftdb.getCursorY();
    else
        y = _tft.getCursorY();
    PUSH(y);
    return 1;
}

static int TFT_set_text_color(stack_t *&sp) {
    uint32_t color = POP();
    if (isBuffered)
        _tftdb.setTextColor(color);
    else
        _tft.setTextColor(color);
    return 1;
}

static int TFT_draw_line(stack_t *&sp) {
    uint32_t color = POP();
    int32_t ye = stack_to_int(POP());
    int32_t xe = stack_to_int(POP());
    int32_t ys = stack_to_int(POP());
    int32_t xs = stack_to_int(POP());
    if (isBuffered)
        _tftdb.drawLine(xs, ys, xe, ye, color);
    else
        _tft.drawLine(xs, ys, xe, ye, color);
    return 1;
}

static int TFT_draw_vline(stack_t *&sp) {
    uint32_t color = POP();
    uint32_t h = POP();
    int32_t ys = stack_to_int(POP());
    int32_t xs = stack_to_int(POP());
    if (isBuffered)
        _tftdb.drawFastVLine(xs, ys, h, color);
    else
        _tft.drawFastVLine(xs, ys, h, color);
    return 1;
}

static int TFT_draw_hline(stack_t *&sp) {
    uint32_t color = POP();
    uint32_t w = POP();
    int32_t ys = stack_to_int(POP());
    int32_t xs = stack_to_int(POP());
    if (isBuffered)
        _tftdb.drawFastHLine(xs, ys, w, color);
    else
        _tft.drawFastHLine(xs, ys, w, color);
    return 1;
}

static int TFT_draw_rect(stack_t *&sp) {
    uint32_t color = POP();
    uint32_t h = POP();
    uint32_t w = POP();
    uint32_t y = stack_to_int(POP());
    uint32_t x = stack_to_int(POP());
    if (isBuffered)
        _tftdb.drawRect(x, y, w, h, color);
    else
        _tft.drawRect(x, y, w, h, color);
    return 1;
}

static int TFT_fill_rect(stack_t *&sp) {
    uint32_t color = POP();
    uint32_t h = POP();
    uint32_t w = POP();
    uint32_t y = stack_to_int(POP());
    uint32_t x = stack_to_int(POP());
    if (isBuffered)
        _tftdb.fillRect(x, y, w, h, color);
    else
        _tft.fillRect(x, y, w, h, color);
    return 1;
}

static int TFT_draw_circle(stack_t *&sp) {
    uint32_t color = POP();
    uint32_t r = POP();
    uint32_t y = stack_to_int(POP());
    uint32_t x = stack_to_int(POP());
    if (isBuffered)
        _tftdb.drawCircle(x, y, r, color);
    else
        _tft.drawCircle(x, y, r, color);
    return 1;
}

static int TFT_fill_circle(stack_t *&sp) {
    uint32_t color = POP();
    uint32_t r = POP();
    uint32_t y = stack_to_int(POP());
    uint32_t x = stack_to_int(POP());
    if (isBuffered)
        _tftdb.fillCircle(x, y, r, color);
    else
        _tft.fillCircle(x, y, r, color);
    return 1;
}

static int TFT_draw_triangle(stack_t *&sp) {
    uint32_t color = POP();
    uint32_t y3 = stack_to_int(POP());
    uint32_t x3 = stack_to_int(POP());
    uint32_t y2 = stack_to_int(POP());
    uint32_t x2 = stack_to_int(POP());
    uint32_t y1 = stack_to_int(POP());
    uint32_t x1 = stack_to_int(POP());
    if (isBuffered)
        _tftdb.drawTriangle(x1, y1, x2, y2, x3, y3, color);
    else
        _tft.drawTriangle(x1, y1, x2, y2, x3, y3, color);
    return 1;
}

static int TFT_fill_triangle(stack_t *&sp) {
    uint32_t color = POP();
    uint32_t y3 = stack_to_int(POP());
    uint32_t x3 = stack_to_int(POP());
    uint32_t y2 = stack_to_int(POP());
    uint32_t x2 = stack_to_int(POP());
    uint32_t y1 = stack_to_int(POP());
    uint32_t x1 = stack_to_int(POP());
    if (isBuffered)
        _tftdb.fillTriangle(x1, y1, x2, y2, x3, y3, color);
    else
        _tft.fillTriangle(x1, y1, x2, y2, x3, y3, color);
    return 1;
}

static int TFT_draw_buffer(stack_t *&sp) {
    if (isBuffered)
        _tftdb.pushSprite(0, 0);
    return 1;
}

static int TFT_rgb_to_565(stack_t *&sp) {
    uint32_t b = POP();
    uint32_t g = POP();
    uint32_t r = POP();
    uint32_t code = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
    PUSH(code);
    return 1;
}

static int TFT_set_text_font(stack_t *&sp) {
    uint32_t font = POP();
    font = constrain(font, 1, 20);
    if (font <= 9) {
        if (isBuffered)
            _tftdb.setTextFont(font);
        else
            _tft.setTextFont(font);
        
    } else {
#ifdef LOAD_GFXFF
        const GFXfont *f = nullptr;
        switch (font) {
            case 10: f = &FreeMono9pt7b; break;
            case 11: f = &FreeMono12pt7b; break;
            case 12: f = &FreeSerif9pt7b; break;
            case 13: f = &FreeSerif12pt7b; break;
            case 14: f = &FreeSerifBold12pt7b; break;
            case 15: f = &FreeSerifItalic12pt7b; break;
            case 16: f = &FreeSerifBoldItalic12pt7b; break;
            case 17: f = &FreeSans12pt7b; break;
            case 18: f = &FreeSans18pt7b; break;
            case 19: f = &FreeSans24pt7b; break;
            case 20: f = &FreeSerifBoldItalic12pt7b; break;
            default:
                _tft.setTextFont(1);
                return 1;
        }
        if (isBuffered)
            _tftdb.setFreeFont(f);
        else
            _tft.setFreeFont(f);
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
    {"init", TFT_init, 2, new VarType::Std[2] {
        VarType::Std::None, VarType::Std::Int
    }},
    /* tft.init_buffered(int rotation) */
    {"init_buffered", TFT_init_buffered, 2, new VarType::Std[2] {
        VarType::Std::None, VarType::Std::Int
    }},
    /* tft.fill_screen(int color) */
    {"fill_screen", TFT_fill_screen, 2, new VarType::Std[2] {
        VarType::Std::None, VarType::Std::Int
    }},
    /* tft.write(string text) */
    {"write", TFT_write, 2, new VarType::Std[2] {
        VarType::Std::None, VarType::Std::String
    }},
    /* tft.draw_string(string text, int x, int y, int font) */
    {"draw_string", TFT_draw_string, 5, new VarType::Std[5] {
        VarType::Std::None, VarType::Std::String, VarType::Std::Int, VarType::Std::Int, VarType::Std::Int
    }},
    /* tft.draw_centre_string(string text, int x, int y, int font) */
    {"draw_centre_string", TFT_draw_centre_string, 5, new VarType::Std[5] {
        VarType::Std::None, VarType::Std::String, VarType::Std::Int, VarType::Std::Int, VarType::Std::Int
    }},
    /* tft.draw_pixel(int width, int height, int color) */
    {"draw_pixel", TFT_draw_pixel, 4, new VarType::Std[4] {
        VarType::Std::None, VarType::Std::Int, VarType::Std::Int, VarType::Std::Int
    }},
    /* int tft.width() */
    {"width", TFT_width, 1, new VarType::Std[1] {
        VarType::Std::Int
    }},
    /* int tft.height() */
    {"height", TFT_height, 1, new VarType::Std[1] {
        VarType::Std::Int
    }},
    /* tft.set_cursor(int x, int y) */
    {"set_cursor", TFT_set_cursor, 3, new VarType::Std[3] {
        VarType::Std::None, VarType::Std::Int, VarType::Std::Int
    }},
    /* int tft.get_cursor_x() */
    {"get_cursor_x", TFT_get_cursor_x, 1, new VarType::Std[1] {
        VarType::Std::Int
    }},
    /* int tft.get_cursor_y() */
    {"get_cursor_y", TFT_get_cursor_y, 1, new VarType::Std[1] {
        VarType::Std::Int
    }},
    /* tft.set_text_color(int color) */
    {"set_text_color", TFT_set_text_color, 2, new VarType::Std[2] {
        VarType::Std::None, VarType::Std::Int
    }},
    /* tft.draw_line(int xs, int ys, int xe, int ye, int color) */
    {"draw_line", TFT_draw_line, 6, new VarType::Std[6] {
        VarType::Std::None, VarType::Std::Int, VarType::Std::Int, VarType::Std::Int, VarType::Std::Int, VarType::Std::Int
    }},
    /* tft.draw_vline(int xs, int ys, int height, int color) */
    {"draw_vline", TFT_draw_vline, 5, new VarType::Std[5] {
        VarType::Std::None, VarType::Std::Int, VarType::Std::Int, VarType::Std::Int, VarType::Std::Int
    }},
    /* tft.draw_hline(int xs, int ys, int width, int color) */
    {"draw_hline", TFT_draw_hline, 5, new VarType::Std[5] {
        VarType::Std::None, VarType::Std::Int, VarType::Std::Int, VarType::Std::Int, VarType::Std::Int
    }},
    /* tft.draw_rect(int x, int y, int w, int h, int color) */
    {"draw_rect", TFT_draw_rect, 6, new VarType::Std[6] {
        VarType::Std::None, VarType::Std::Int, VarType::Std::Int, VarType::Std::Int, VarType::Std::Int, VarType::Std::Int
    }},
    /* tft.fill_rect(int x, int y, int w, int h, int color) */
    {"fill_rect", TFT_fill_rect, 6, new VarType::Std[6] {
        VarType::Std::None, VarType::Std::Int, VarType::Std::Int, VarType::Std::Int, VarType::Std::Int, VarType::Std::Int
    }},
    /* tft.draw_circle(int x, int y, int r, int color) */
    {"draw_circle", TFT_draw_circle, 5, new VarType::Std[5] {
        VarType::Std::None, VarType::Std::Int, VarType::Std::Int, VarType::Std::Int, VarType::Std::Int
    }},
    /* tft.fill_circle(int x, int y, int r, int color) */
    {"fill_circle", TFT_fill_circle, 5, new VarType::Std[5] {
        VarType::Std::None, VarType::Std::Int, VarType::Std::Int, VarType::Std::Int, VarType::Std::Int
    }},
    /* tft.draw_triangle(int x1, int y1, int x2, int y2, int x3, int y3, int color) */
    {"draw_triangle", TFT_draw_triangle, 8, new VarType::Std[8] {
        VarType::Std::None, VarType::Std::Int, VarType::Std::Int, VarType::Std::Int, VarType::Std::Int,
        VarType::Std::Int, VarType::Std::Int, VarType::Std::Int
    }},
    /* tft.fill_triangle(int x1, int y1, int x2, int y2, int x3, int y3, int color) */
    {"fill_triangle", TFT_fill_triangle, 8, new VarType::Std[8] {
        VarType::Std::None, VarType::Std::Int, VarType::Std::Int, VarType::Std::Int, VarType::Std::Int,
        VarType::Std::Int, VarType::Std::Int, VarType::Std::Int
    }},
    /* int tft.rgb_to_565(int r, int g, int b) */
    {"rgb", TFT_rgb_to_565, 4, new VarType::Std[4] {
        VarType::Std::Int, VarType::Std::Int, VarType::Std::Int, VarType::Std::Int
    }},
    /* tft.set_font(int font) */
    {"set_font", TFT_set_text_font, 2, new VarType::Std[2] {
        VarType::Std::None, VarType::Std::Int
    }},
    /* tft.draw_image(int_array image, int x, int y) */
    {"draw_image", TFT_draw_image, 4, new VarType::Std[4] {
        VarType::Std::None, VarType::Std::String, VarType::Std::Int, VarType::Std::Int
    }},
    {"draw_buffer", TFT_draw_buffer, 1, new VarType::Std[1] {
        VarType::Std::None
    }},
    {"delete_buffer", TFT_end, 1, new VarType::Std[1] {
        VarType::Std::None
    }},
#ifdef SCR_CONFIG_TFT_LIB_ENABLE_JPEG_MODULE
    /* tft.draw_sd_jpeg(string filename, int x, int y) */
    {"draw_sd_jpeg", TFT_draw_sd_jpeg, 4, new VarType::Std[4] {
        VarType::Std::None, VarType::Std::String, VarType::Std::Int, VarType::Std::Int
    }}
#endif
};

#endif // LIB_TFT_ESP32_H