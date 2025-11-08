#include "stdint.h"
#include "Adafruit_GFX.h"
#include "lpm012m134b.h"

#define LCD_BL 14

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
      if (s < flush_start) flush_start = s;
      flush_height = max(s + h, flush_height + flush_start) - flush_start;
    }

    void drawPixel(int16_t x, int16_t y, uint16_t color) {
      soft_rotation(x, y);
      lpm.drawPixel(x, y, lpm.rgb565_to_rgb222(color));
      // lpm.drawPixelRGB565(x, y, color);
      lpm.flush(y, 1);
    }

    void startWrite() {
      flush_start = 240;
      flush_height = 0;
    }

    void writePixel(int16_t x, int16_t y, uint16_t color) {
      soft_rotation(x, y);
      lpm.drawPixel(x, y, lpm.rgb565_to_rgb222(color));
      // lpm.drawPixelRGB565(x, y, color);
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
      lpm.drawRect(x, y, w, h, lpm.rgb565_to_rgb222(color));
      // lpm.drawRectRGB565(x, y, w, h, color);
      update_flush_area(y, h);
    }

    void writeFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {
      soft_rotation(x, y);
      if (rotation == 0 || rotation == 2) {
        if (rotation == 2) y = y - h + 1;
        lpm.drawFastVLine(x, y, h, lpm.rgb565_to_rgb222(color));
        update_flush_area(y, h);
      }
      else {
        if (rotation == 3) x = x - h + 1;
        lpm.drawFastHLine(x, y, h, lpm.rgb565_to_rgb222(color));
        // lpm.drawFastVLineRGB565(x, y, h, color);
        update_flush_area(y, 1);
      }
    }

    void writeFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
      soft_rotation(x, y);
      if (rotation == 0 || rotation == 2) {
        if (rotation == 2) x = x - w + 1;
        lpm.drawFastHLine(x, y, w, lpm.rgb565_to_rgb222(color));
        update_flush_area(y, 1);
      }
      else {
        if (rotation == 3) y = y - w + 1;
        lpm.drawFastVLine(x, y, w, lpm.rgb565_to_rgb222(color));
        // lpm.drawFastHLineRGB565(x, y, w, color);
        update_flush_area(y, w);
      }
    }

    void writeLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
      soft_rotation(x0, y0);
      soft_rotation(x1, y1);
      lpm.drawLine(x0, y0, x1, y1, lpm.rgb565_to_rgb222(color));
      // lpm.drawLineRGB565(x0, y0, x1, y1, color);
      update_flush_area(min(y0, y1), abs(y1 - y0) + 1);
    }

    void endWrite(void) {
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
      lpm.fill(lpm.rgb565_to_rgb222(color));
      // lpm.fillRGB565(color);
      update_flush_area(0, 240);
      endWrite();
    }

    void setRotation(uint8_t r) {
      rotation = r;
    }

    uint16_t color565(uint8_t red, uint8_t green, uint8_t blue) {
      return ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | (blue >> 3);
    }
};

Adafruit_GFX_LPM012M134B tft = Adafruit_GFX_LPM012M134B(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13);

void setup() {
  Serial.begin();
  delay(1000);
  Serial.println("LPM012M134B Test!"); 

  pinMode(LCD_BL, OUTPUT);
  digitalWrite(LCD_BL, HIGH);
 
  tft.begin();
  
  Serial.println(F("Benchmark                Time (microseconds)"));
  delay(10);
  Serial.print(F("Screen fill              "));
  Serial.println(testFillScreen());
  delay(500);

  Serial.print(F("Text                     "));
  Serial.println(testText());
  delay(3000);

  Serial.print(F("Lines                    "));
  Serial.println(testLines(RGB565_CYAN));
  delay(500);

  Serial.print(F("Horiz/Vert Lines         "));
  Serial.println(testFastLines(RGB565_RED, RGB565_BLUE));
  delay(500);

  Serial.print(F("Rectangles (outline)     "));
  Serial.println(testRects(RGB565_GREEN));
  delay(500);

  Serial.print(F("Rectangles (filled)      "));
  Serial.println(testFilledRects(RGB565_YELLOW, RGB565_MAGENTA));
  delay(500);

  Serial.print(F("Circles (filled)         "));
  Serial.println(testFilledCircles(10, RGB565_MAGENTA));

  Serial.print(F("Circles (outline)        "));
  Serial.println(testCircles(10, RGB565_WHITE));
  delay(500);

  Serial.print(F("Triangles (outline)      "));
  Serial.println(testTriangles());
  delay(500);

  Serial.print(F("Triangles (filled)       "));
  Serial.println(testFilledTriangles());
  delay(500);

  Serial.print(F("Rounded rects (outline)  "));
  Serial.println(testRoundRects());
  delay(500);

  Serial.print(F("Rounded rects (filled)   "));
  Serial.println(testFilledRoundRects());
  delay(500);

  Serial.println(F("Done!"));

}


void loop(void) {
  for(uint8_t rotation=0; rotation<4; rotation++) {
    tft.setRotation(rotation);
    testText();
    testTriangles();
    delay(1000);
  }
}

unsigned long testFillScreen() {
  unsigned long start = micros();
  tft.fillScreen(RGB565_BLACK);
  yield();
  tft.fillScreen(RGB565_RED);
  yield();
  tft.fillScreen(RGB565_GREEN);
  yield();
  tft.fillScreen(RGB565_BLUE);
  yield();
  tft.fillScreen(RGB565_BLACK);
  yield();
  return micros() - start;
}

unsigned long testText() {
  tft.setFont();
  tft.fillScreen(RGB565_BLACK);
  unsigned long start = micros();
  tft.setCursor(0, 0);
  tft.setTextColor(RGB565_WHITE);  tft.setTextSize(1);
  tft.println("Hello World!");
  tft.setTextColor(RGB565_YELLOW); tft.setTextSize(2);
  tft.println(1234.56);
  tft.setTextColor(RGB565_RED);    tft.setTextSize(3);
  tft.println(0xDEADBEEF, HEX);
  tft.println();
  tft.setTextColor(RGB565_GREEN);
  tft.setTextSize(5);
  tft.println("Groop");
  tft.setTextSize(2);
  tft.println("I implore thee,");
  tft.setTextSize(1);
  tft.println("my foonting turlingdromes.");
  tft.println("And hooptiously drangle me");
  tft.println("with crinkly bindlewurdles,");
  tft.println("Or I will rend thee");
  tft.println("in the gobberwarts");
  tft.println("with my blurglecruncheon,");
  tft.println("see if I don't!");
  return micros() - start;
}

unsigned long testLines(uint16_t color) {
  unsigned long start, t;
  int           x1, y1, x2, y2,
                w = tft.width(),
                h = tft.height();

  tft.fillScreen(RGB565_BLACK);
  yield();
  
  x1 = y1 = 0;
  y2    = h - 1;
  start = micros();
  for(x2=0; x2<w; x2+=6) tft.drawLine(x1, y1, x2, y2, color);
  x2    = w - 1;
  for(y2=0; y2<h; y2+=6) tft.drawLine(x1, y1, x2, y2, color);
  t     = micros() - start; // fillScreen doesn't count against timing

  yield();
  tft.fillScreen(RGB565_BLACK);
  yield();

  x1    = w - 1;
  y1    = 0;
  y2    = h - 1;
  start = micros();
  for(x2=0; x2<w; x2+=6) tft.drawLine(x1, y1, x2, y2, color);
  x2    = 0;
  for(y2=0; y2<h; y2+=6) tft.drawLine(x1, y1, x2, y2, color);
  t    += micros() - start;

  yield();
  tft.fillScreen(RGB565_BLACK);
  yield();

  x1    = 0;
  y1    = h - 1;
  y2    = 0;
  start = micros();
  for(x2=0; x2<w; x2+=6) tft.drawLine(x1, y1, x2, y2, color);
  x2    = w - 1;
  for(y2=0; y2<h; y2+=6) tft.drawLine(x1, y1, x2, y2, color);
  t    += micros() - start;

  yield();
  tft.fillScreen(RGB565_BLACK);
  yield();

  x1    = w - 1;
  y1    = h - 1;
  y2    = 0;
  start = micros();
  for(x2=0; x2<w; x2+=6) tft.drawLine(x1, y1, x2, y2, color);
  x2    = 0;
  for(y2=0; y2<h; y2+=6) tft.drawLine(x1, y1, x2, y2, color);

  yield();
  return micros() - start;
}

unsigned long testFastLines(uint16_t color1, uint16_t color2) {
  unsigned long start;
  int           x, y, w = tft.width(), h = tft.height();

  tft.fillScreen(RGB565_BLACK);
  start = micros();
  for(y=0; y<h; y+=5) tft.drawFastHLine(0, y, w, color1);
  for(x=0; x<w; x+=5) tft.drawFastVLine(x, 0, h, color2);

  return micros() - start;
}

unsigned long testRects(uint16_t color) {
  unsigned long start;
  int           n, i, i2,
                cx = tft.width()  / 2,
                cy = tft.height() / 2;

  tft.fillScreen(RGB565_BLACK);
  n     = min(tft.width(), tft.height());
  start = micros();
  for(i=2; i<n; i+=6) {
    i2 = i / 2;
    tft.drawRect(cx-i2, cy-i2, i, i, color);
  }

  return micros() - start;
}

unsigned long testFilledRects(uint16_t color1, uint16_t color2) {
  unsigned long start, t = 0;
  int           n, i, i2,
                cx = tft.width()  / 2 - 1,
                cy = tft.height() / 2 - 1;

  tft.fillScreen(RGB565_BLACK);
  n = min(tft.width(), tft.height());
  for(i=n; i>0; i-=6) {
    i2    = i / 2;
    start = micros();
    tft.fillRect(cx-i2, cy-i2, i, i, color1);
    t    += micros() - start;
    // Outlines are not included in timing results
    tft.drawRect(cx-i2, cy-i2, i, i, color2);
    yield();
  }

  return t;
}

unsigned long testFilledCircles(uint8_t radius, uint16_t color) {
  unsigned long start;
  int x, y, w = tft.width(), h = tft.height(), r2 = radius * 2;

  tft.fillScreen(RGB565_BLACK);
  start = micros();
  for(x=radius; x<w; x+=r2) {
    for(y=radius; y<h; y+=r2) {
      tft.fillCircle(x, y, radius, color);
    }
  }

  return micros() - start;
}

unsigned long testCircles(uint8_t radius, uint16_t color) {
  unsigned long start;
  int           x, y, r2 = radius * 2,
                w = tft.width()  + radius,
                h = tft.height() + radius;

  // Screen is not cleared for this one -- this is
  // intentional and does not affect the reported time.
  start = micros();
  for(x=0; x<w; x+=r2) {
    for(y=0; y<h; y+=r2) {
      tft.drawCircle(x, y, radius, color);
    }
  }

  return micros() - start;
}

unsigned long testTriangles() {
  unsigned long start;
  int           n, i, cx = tft.width()  / 2 - 1,
                      cy = tft.height() / 2 - 1;

  tft.fillScreen(RGB565_BLACK);
  n     = min(cx, cy);
  start = micros();
  for(i=0; i<n; i+=5) {
    tft.drawTriangle(
      cx    , cy - i, // peak
      cx - i, cy + i, // bottom left
      cx + i, cy + i, // bottom right
      tft.color565(i, i, i));
  }

  return micros() - start;
}

unsigned long testFilledTriangles() {
  unsigned long start, t = 0;
  int           i, cx = tft.width()  / 2 - 1,
                   cy = tft.height() / 2 - 1;

  tft.fillScreen(RGB565_BLACK);
  start = micros();
  for(i=min(cx,cy); i>10; i-=5) {
    start = micros();
    tft.fillTriangle(cx, cy - i, cx - i, cy + i, cx + i, cy + i,
      tft.color565(0, i*10, i*10));
    t += micros() - start;
    tft.drawTriangle(cx, cy - i, cx - i, cy + i, cx + i, cy + i,
      tft.color565(i*10, i*10, 0));
    yield();
  }

  return t;
}

unsigned long testRoundRects() {
  unsigned long start;
  int           w, i, i2,
                cx = tft.width()  / 2 - 1,
                cy = tft.height() / 2 - 1;

  tft.fillScreen(RGB565_BLACK);
  w     = min(tft.width(), tft.height());
  start = micros();
  for(i=0; i<w; i+=6) {
    i2 = i / 2;
    tft.drawRoundRect(cx-i2, cy-i2, i, i, i/8, tft.color565(i, 0, 0));
  }

  return micros() - start;
}

unsigned long testFilledRoundRects() {
  unsigned long start;
  int           i, i2,
                cx = tft.width()  / 2 - 1,
                cy = tft.height() / 2 - 1;

  tft.fillScreen(RGB565_BLACK);
  start = micros();
  for(i=min(tft.width(), tft.height()); i>20; i-=6) {
    i2 = i / 2;
    tft.fillRoundRect(cx-i2, cy-i2, i, i, i/8, tft.color565(0, i, 0));
    yield();
  }

  return micros() - start;
}
