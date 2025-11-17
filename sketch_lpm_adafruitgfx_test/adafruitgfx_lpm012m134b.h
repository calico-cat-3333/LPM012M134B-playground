#include "stdint.h"
#include "Adafruit_GFX.h"
#include "lpm012m134b.h"

#define RGB565_BLACK 0x0000       ///<   0,   0,   0
#define RGB565_NAVY 0x000F        ///<   0,   0, 123
#define RGB565_DARKGREEN 0x03E0   ///<   0, 125,   0
#define RGB565_DARKCYAN 0x03EF    ///<   0, 125, 123
#define RGB565_MAROON 0x7800      ///< 123,   0,   0
#define RGB565_PURPLE 0x780F      ///< 123,   0, 123FF
#define RGB565_OLIVE 0x7BE0       ///< 123, 125,   0
#define RGB565_LIGHTGREY 0xC618   ///< 198, 195, 198
#define RGB565_DARKGREY 0x7BEF    ///< 123, 125, 123
#define RGB565_BLUE 0x001F        ///<   0,   0, 255
#define RGB565_GREEN 0x07E0       ///<   0, 255,   0
#define RGB565_CYAN 0x07FF        ///<   0, 255, 255
#define RGB565_RED 0xF800         ///< 255,   0,   0
#define RGB565_MAGENTA 0xF81F     ///< 255,   0, 255
#define RGB565_YELLOW 0xFFE0      ///< 255, 255,   0
#define RGB565_WHITE 0xFFFF       ///< 255, 255, 255
#define RGB565_ORANGE 0xFD20      ///< 255, 165,   0
#define RGB565_GREENYELLOW 0xAFE5 ///< 173, 255,  41
#define RGB565_PINK 0xFC18        ///< 255, 130, 198

class Adafruit_GFX_LPM012M134B: public Adafruit_GFX {
  private:
    LPM012M134B lpm;
    int flush_start;
    int flush_height;
    uint8_t rotation;
    bool use_bayer;

    void soft_rotation(int16_t &x, int16_t &y) {
      int xo = x;
      int yo = y;
      if (rotation == 0) return;
      else if (rotation == 1) {
        y = xo;
        x = _width - yo - 1;
      }
      else if (rotation == 2) {
        y = _height - yo - 1;
        x = _width - xo - 1;
      }
      else if (rotation == 3) {
        y = _height - xo - 1;
        x = yo;
      }
    }
    
  public:
    Adafruit_GFX_LPM012M134B(int vst, int vck, int enb, int xrst, int frp, int xfrp, int hst, int hck,
                int r1, int r2, int g1, int g2, int b1, int b2): Adafruit_GFX(240, 240),
                lpm(vst, vck, enb, xrst, frp, xfrp, hst, hck, r1, r2, g1, g2, b1, b2) {
    }

    void begin() {
      lpm.init();
      lpm.fill(0);
      rotation = 0;
    }

    void update_flush_area(int s, int h) {
      if (flush_height != 0) flush_height = flush_height + flush_start; // actually this is endy
      if (s < flush_start) flush_start = s;
      flush_height = max(s + h, flush_height) - flush_start;
    }

    void drawPixel(int16_t x, int16_t y, uint16_t color) {
      soft_rotation(x, y);
      if (!use_bayer) lpm.drawPixel(x, y, lpm.rgb565_to_rgb222(color));
      else lpm.drawPixelRGB565(x, y, color);
      lpm.flush(y, 1);
    }

    void startWrite() {
      flush_start = 240;
      flush_height = 0;
    }

    void writePixel(int16_t x, int16_t y, uint16_t color) {
      soft_rotation(x, y);
      if (!use_bayer) lpm.drawPixel(x, y, lpm.rgb565_to_rgb222(color));
      else lpm.drawPixelRGB565(x, y, color);
      update_flush_area(y, 1);
    }

    void writeFillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
      soft_rotation(x, y);
      if (rotation == 1 || rotation == 3) {
        int t = w;
        w = h;
        h = t;
      }
      if (rotation == 1 || rotation == 2) x = x - w + 1;
      if (rotation == 3 || rotation == 2) y = y - h + 1; 
      if (!use_bayer) lpm.drawRect(x, y, w, h, lpm.rgb565_to_rgb222(color));
      else lpm.drawRectRGB565(x, y, w, h, color);
      update_flush_area(y, h);
    }

    void writeFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {
      soft_rotation(x, y);
      if (rotation == 0 || rotation == 2) {
        if (rotation == 2) y = y - h + 1;
        if (!use_bayer) lpm.drawFastVLine(x, y, h, lpm.rgb565_to_rgb222(color));
        else lpm.drawFastVLineRGB565(x, y, h, color);
        update_flush_area(y, h);
      }
      else {
        if (rotation == 1) x = x - h + 1;
        if (!use_bayer) lpm.drawFastHLine(x, y, h, lpm.rgb565_to_rgb222(color));
        else lpm.drawFastHLineRGB565(x, y, h, color);
        update_flush_area(y, 1);
      }
    }

    void writeFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
      soft_rotation(x, y);
      if (rotation == 0 || rotation == 2) {
        if (rotation == 2) x = x - w + 1;
        if (!use_bayer) lpm.drawFastHLine(x, y, w, lpm.rgb565_to_rgb222(color));
        else lpm.drawFastHLineRGB565(x, y, w, color);
        update_flush_area(y, 1);
      }
      else {
        if (rotation == 3) y = y - w + 1;
        if (!use_bayer) lpm.drawFastVLine(x, y, w, lpm.rgb565_to_rgb222(color));
        else lpm.drawFastVLineRGB565(x, y, w, color);
        update_flush_area(y, w);
      }
    }

    void writeLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
      soft_rotation(x0, y0);
      soft_rotation(x1, y1);
      if (!use_bayer) lpm.drawLine(x0, y0, x1, y1, lpm.rgb565_to_rgb222(color));
      else lpm.drawLineRGB565(x0, y0, x1, y1, color);
      update_flush_area(min(y0, y1), abs(y1 - y0) + 1);
    }

    void endWrite(void) {
      // Serial.print(rotation);
      // Serial.print(' ');
      // Serial.print(flush_start);
      // Serial.print(' ');
      // Serial.println(flush_height);
      lpm.flush(flush_start, flush_height);
      // lpm.flush();
    }

    void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {
      startWrite();
      writeFastVLine(x, y, h, color);
      endWrite();
    }

    void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
      startWrite();
      writeFastHLine(x, y, w, color);
      endWrite();
    }

    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
      startWrite();
      writeFillRect(x, y, w, h, color);
      endWrite();
    }

    void fillScreen(uint16_t color) {
      startWrite();
      if (!use_bayer) lpm.fill(lpm.rgb565_to_rgb222(color));
      else lpm.fillRGB565(color);
      update_flush_area(0, 240);
      endWrite();
    }

    void setRotation(uint8_t r) {
      rotation = r;
    }

    void setEnableBayerDither(bool en) {
      use_bayer = en;
    }

    uint16_t color565(uint8_t red, uint8_t green, uint8_t blue) {
      return ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | (blue >> 3);
    }
};
