// ===================================================================================
// SSD1306 128x64 Pixels OLED Minimal Functions                               * v1.0 *
// ===================================================================================
//
// Collection of the most necessary functions for controlling an SSD1306 128x64 pixels
// I2C OLED.
//
// References:
// -----------
// - TinyOLEDdemo: https://github.com/wagiminator/attiny13-tinyoleddemo
// - Neven Boyanov: https://github.com/tinusaur/ssd1306xled
// - Stephen Denne: https://github.com/datacute/Tiny4kOLED
//
// 2022 by Stefan Wagner: https://github.com/wagiminator

#include "oled_min.h"

// OLED initialisation sequence
const uint8_t OLED_INIT_CMD[] = {
  OLED_MULTIPLEX,   0x3F,                 // set multiplex ratio  
  OLED_CHARGEPUMP,  0x14,                 // set DC-DC enable  
  OLED_MEMORYMODE,  0x00,                 // set horizontal addressing mode
  OLED_COLUMNS,     0x00, 0x7F,           // set start and end column
  OLED_PAGES,       0x00, 0x3F,           // set start and end page
  OLED_COMPINS,     0x12,                 // set com pins
  OLED_XFLIP, OLED_YFLIP,                 // flip screen
  OLED_DISPLAY_ON                         // display on
};

// OLED init function
void OLED_init(void) {
  uint8_t i;
  I2C_init();                             // initialize I2C first
  I2C_start(OLED_ADDR);                   // start transmission to OLED
  I2C_write(OLED_CMD_MODE);               // set command mode
  for(i = 0; i < sizeof(OLED_INIT_CMD); i++)
    I2C_write(OLED_INIT_CMD[i]);          // send the command bytes
  I2C_stop();                             // stop transmission
}

// Start sending data
void OLED_data_start(void) {
  I2C_start(OLED_ADDR);                   // start transmission to OLED
  I2C_write(OLED_DAT_MODE);               // set data mode
}

// Start sending command
void OLED_command_start(void) {
  I2C_start(OLED_ADDR);                   // start transmission to OLED
  I2C_write(OLED_CMD_MODE);               // set command mode
}

// OLED send command
void OLED_send_command(uint8_t cmd) {
  I2C_start(OLED_ADDR);                   // start transmission to OLED
  I2C_write(OLED_CMD_MODE);               // set command mode
  I2C_write(cmd);                         // send command
  I2C_stop();                             // stop transmission
}

// OLED set cursor position
void OLED_setpos(uint8_t x, uint8_t y) {
  I2C_start(OLED_ADDR);                   // start transmission to OLED
  I2C_write(OLED_CMD_MODE);               // set command mode
  I2C_write(OLED_PAGE | y);	              // set page start address
  I2C_write(x & 0x0F);			              // set lower nibble of start column
  I2C_write(OLED_COLUMN_HIGH | (x >> 4)); // set higher nibble of start column
  I2C_stop();                             // stop transmission
}

// OLED fill screen
void OLED_fill(uint8_t p) {
  OLED_setpos(0, 0);                      // set cursor to display start
  I2C_start(OLED_ADDR);                   // start transmission to OLED
  I2C_write(OLED_DAT_MODE);               // set data mode
  for(uint16_t i=128*8; i; i--) I2C_write(p); // send pattern
  I2C_stop();                             // stop transmission
}

// OLED draw bitmap
void OLED_draw_bmp(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, const uint8_t* bmp) {
  for(uint8_t y = y0; y < y1; y++) {
    OLED_setpos(x0, y);
    I2C_start(OLED_ADDR);
    I2C_write(OLED_DAT_MODE);
    for(uint8_t x = x0; x < x1; x++)
      I2C_write(*bmp++);
    I2C_stop();
  }
}


#define display_init            OLED_init
#define display_end             I2C_stop
#define display_send(b)         I2C_write(b)
#define display_send_cmd(c)     OLED_send_command(c)
#define display_data_start(y)   { OLED_setpos(0, y); OLED_data_start(); }

#include "PixelOperator.h"
#define font_data FONT8X16PO
#include <stdint.h>
#include <string.h>

#define u8 uint8_t
#define bool int8_t

#define BUF_LEN (128 * 64 / 8)
static uint8_t display_buffer[BUF_LEN] = { 0 };

void display_clear() {
    memset(display_buffer, 0, BUF_LEN);
}

void display_flush() {
    OLED_setpos(0, 0);
    for (int i = 0; i < BUF_LEN; ++i) {
        if (i % 16 == 0) {
            if (i > 0) {
                OLED_data_stop();
            }
            OLED_data_start();
        }
        I2C_write(display_buffer[i]);
    }
    OLED_data_stop();
}

void display_draw_dot(int x, int y) {
    int row = y / 8;
    int offset = row * 128 + x;
    int pixel_offset = y % 8;
    u8 b = display_buffer[offset];
    b = b | (1 << pixel_offset);
    display_buffer[offset] = b;
}

void display_draw_xbmp(int x, int y, int w, int h, const uint8_t *data) {
    int byte_offset = y % 8;
    for (int tx = 0; tx < w; ++tx) {
        int px = tx + x;
        if (px >= 128) {
            break;
        }
        if (px < 0) {
            continue;
        }
        uint8_t prev_b = 0;
        int ty_len = (h / 8) + (h % 8 > 0 ? 1 : 0);
        int py = 0;
        for (int ty = 0; ty < ty_len; ++ty) {
            py = y / 8 + ty;
            if (py >= 8) {
                continue;
            }
            uint8_t b = *(data + (ty * w + tx));
            uint8_t b2 = b;
            b = (b << byte_offset) | prev_b;
            display_buffer[py * 128 + px] |= b;
            prev_b = b2 >> (7 - byte_offset);
        }
        py = y / 8 + ty_len;
        if (prev_b > 0 && py < 8) {
            display_buffer[py * 128 + px] |= prev_b;
        }
    }
}

void display_draw_str(int x, int y, const char *str) {
    uint8_t chidx = 0;
    int charbytes = font_data->width * font_data->height;
    for (; str[chidx] != 0; ++chidx) {
        uint8_t ch = str[chidx];
        if (ch >= font_data->min && ch <= font_data->max) {
            display_draw_xbmp(x + font_data->width * chidx, y, font_data->width, font_data->height * 8, font_data->data + (ch - font_data->min) * charbytes);
        }
    }
}
