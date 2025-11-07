#include "stdint.h"
#include "lpm012m134b.h"

#define LCD_BL 14

LPM012M134B lpm(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13);

void setup() {
  // put your setup code here, to run once:
  Serial.begin();

  pinMode(LCD_BL, OUTPUT);
  digitalWrite(LCD_BL, HIGH);

  randomSeed(micros());
  lpm.init();
  lpm.fill(0);
  lpm.flush();
}

int cr = 0;
int8_t color = 0;
int fs = 0;
int hi = 240;

void fillTestStep() {
	lpm.fill(color);
	fs = 0;
	hi = 240;
}

void drawPixelTestStep() {
  int x = (int)random(40, 200);
  int y = (int)random(40, 200);
  lpm.drawPixel(x, y, color);
  fs = y;
  hi = 1;
}

void drawFastHLineTestStep() {
  int x = (int)random(0, 200);
  int y = (int)random(0, 240);
  int w = (int)random(40, 240);
  lpm.drawFastHLine(x, y, w, color);
  fs = y;
  hi = 1;
}

void drawFastVLineTestStep() {
  int x = (int)random(0, 240);
  int y = (int)random(0, 200);
  int h = (int)random(40, 240);
  lpm.drawFastVLine(x, y, h, color);
  fs = y;
  hi = h;
}

void drawLineTestStep() {
  int x0 = (int)random(0, 200);
  int y0 = (int)random(0, 200);
  int x1 = (int)random(40, 200);
  int y1 = (int)random(40, 200);
  lpm.drawLine(x0, y0, x1, y1, color);
  fs = min(y0, y1);
  hi = abs(y0 - y1) + 1;
}

void drawRectTestStep() {
  int x = (int)random(0, 200);
  int y = (int)random(0, 200);
  int w = (int)random(10, 200);
  int h = (int)random(10, 200);
  lpm.drawRect(x, y, w, h, color);
  fs = y;
  hi = h;
}

void drawEllipseTestStep() {
  int x = (int)random(20, 200);
  int y = (int)random(20, 200);
  int a = (int)random(10, 60);
  int b = (int)random(10, 60);
  lpm.drawEllipse(x, y, a, b, color);
  fs = 0;
  hi = 240;
}

uint16_t randomRGB565Color() {
  int r = (int)random(50, 255) & 0b11111;
  int g = (int)random(50, 255) & 0b111111;
  int b = (int)random(50, 255) & 0b11111;
  return (r << 11) | (g << 5) | b;
}

void fillRGB565TestStep() {
	lpm.fillRGB565(randomRGB565Color());
	fs = 0;
	hi = 240;
}

void drawPixelRGB565TestStep() {
  int x = (int)random(40, 200);
  int y = (int)random(40, 200);
  lpm.drawPixelRGB565(x, y, randomRGB565Color());
  fs = y;
  hi = 1;
}

void drawFastHLineRGB565TestStep() {
  int x = (int)random(0, 200);
  int y = (int)random(0, 240);
  int w = (int)random(40, 200);
  lpm.drawFastHLineRGB565(x, y, w, randomRGB565Color());
  fs = y;
  hi = 1;
}

void drawFastVLineRGB565TestStep() {
  int x = (int)random(0, 240);
  int y = (int)random(0, 200);
  int h = (int)random(40, 200);
  lpm.drawFastVLineRGB565(x, y, h, randomRGB565Color());
  fs = y;
  hi = h;
}

void drawLineRGB565TestStep() {
  int x0 = (int)random(0, 200);
  int y0 = (int)random(0, 200);
  int x1 = (int)random(40, 200);
  int y1 = (int)random(40, 200);
  lpm.drawLineRGB565(x0, y0, x1, y1, randomRGB565Color());
  fs = min(y0, y1);
  hi = abs(y0 - y1) + 1;
}

void drawRectRGB565TestStep() {
  int x = (int)random(0, 200);
  int y = (int)random(0, 200);
  int w = (int)random(10, 200);
  int h = (int)random(10, 200);
  lpm.drawRectRGB565(x, y, w, h, randomRGB565Color());
  fs = y;
  hi = h;
}

void drawEllipseRGB565TestStep() {
  int x = (int)random(20, 200);
  int y = (int)random(20, 200);
  int a = (int)random(10, 60);
  int b = (int)random(10, 60);
  lpm.drawEllipseRGB565(x, y, a, b, randomRGB565Color());
  fs = 0;
  hi = 240;
}

void loop() {
  // put your main code here, to run repeatedly:
  fs = 0;
  hi = 240;
  if (cr == 0) {
    drawPixelTestStep();
  }
  if (cr == 1) {
    drawFastHLineTestStep();
  }
  if (cr == 2) {
    drawFastVLineTestStep();
  }
  if (cr == 3) {
    drawLineTestStep();
  }
  if (cr == 4) {
    drawRectTestStep();
  }
  if (cr == 5) {
    drawEllipseTestStep();
  }
  if (cr == 6) {
    drawPixelRGB565TestStep();
  }
  if (cr == 7) {
    drawFastHLineRGB565TestStep();
  }
  if (cr == 8) {
    drawFastVLineRGB565TestStep();
  }
  if (cr == 9) {
    drawLineRGB565TestStep();
  }
  if (cr == 10) {
    drawRectRGB565TestStep();
  }
  if (cr == 11) {
    drawEllipseRGB565TestStep();
  }
  if (cr == 12) {
  	fillTestStep();
  }
  if (cr == 13) {
  	fillRGB565TestStep();
  }
  lpm.flush(fs, hi);
  delay(50);
  color ++;
  if (color == 64) {
    color = 0;
    cr = (cr + 1) % 14;
    delay(1000);
    lpm.fill(0);
    lpm.flush();
  }
}
