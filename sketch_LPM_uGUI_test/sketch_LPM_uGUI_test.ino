#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "lpm012m134b.h"
extern "C" {
  #include "ugui.h"
}

LPM012M134B lpm(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13);
UG_GUI gui;

int ymin = 240;
int ymax = 0;

void pset(UG_S16 x, UG_S16 y, UG_COLOR c) {
  uint8_t rgb222c = ((c & 0xc00000) >> 18) | ((c & 0x00c000) >> 12) | ((c & 0x0000c0) >> 6);
  lpm.drawPixel(x, y, rgb222c);
  ymin = min(ymin, y);
  ymax = max(ymax, y);
}

int bl = 14;
int key = 20;

void setup() {
  // put your setup code here, to run once:
  Serial.begin();

  pinMode(bl, OUTPUT);
  digitalWrite(bl, HIGH);

  pinMode(key, INPUT_PULLUP);

  randomSeed(micros());
  lpm.init();
  UG_Init(&gui, pset, 240, 240);
  UG_FontSelect(&FONT_12X20);
  UG_ConsoleSetArea(65, 30, 175, 210);
  UG_ConsoleSetBackcolor(C_BLACK);
  UG_ConsoleSetForecolor(C_WHITE);
  analogReadResolution(12);
}

UG_COLOR rgb222toc(int rgb222) {
  int r = (rgb222 & 0b110000) << 18;
  int g = (rgb222 & 0b001100) << 12;
  int b = (rgb222 & 0b000011) << 6;
  return r | g | b;
}

int c = 1;
int col = 0;

void allchars() {
  // put your main code here, to run repeatedly:
  char b[] = " ";
  b[0] = char(c);
  c = c + 1;
  if (c == 0xff) c=1;
  UG_ConsoleSetForecolor(rgb222toc(col));
  UG_ConsolePutString(b);
  UG_Update();
  col = (col + 1) % 64;
  int h = ymax - ymin + 1;
  if (h > 0) {
    unsigned long start = micros();
    lpm.flush(ymin, h);
    unsigned long end = micros();
    Serial.print(ymin);
    Serial.print(" ");
    Serial.print(h);
    Serial.print(" flush timeuse ");
    Serial.print(end - start);
    Serial.println(" us");
    ymax = 0;
    ymin = 240;
  }
  delay(300);
}

UG_U8 j, tog;

void loading_anim()  {
  for (UG_U16 sec = 1; sec != 0x100 ; sec <<=1 ) {
    j ++;
    if ( j >=9 ) {
      j = 0 ;
      tog = !tog;
      if (!tog) col = (col + 1) % 64;
    }
    if (tog) {
      for (int i=60; i>=40; i--) UG_DrawArc( 120 , 120 , i , sec , C_BLACK ) ;
    }
    else {
      for (int i=60; i>=40; i--) UG_DrawArc( 120 , 120 , i , sec , rgb222toc(col)) ;
    }
    int h = ymax - ymin + 1;
    if (h > 0) {
      unsigned long start = micros();
      lpm.flush(ymin, h);
      unsigned long end = micros();
      Serial.print(ymin);
      Serial.print(" ");
      Serial.print(h);
      Serial.print(" flush timeuse ");
      Serial.print(end - start);
      Serial.println(" us");
      ymax = 0;
      ymin = 240;
    }
    delay(60);
  }
}

void colors() {
  lpm.fill(col);
  lpm.flush();
  delay(500);
  col = (col + 1) % 64;
}

void randgraphics() {
int t = (int)random(0, 3);
  int x = (int)random(0, 190);
  int y = (int)random(0, 190);
  int w = (int)random(5, 70);
  int h = (int)random(5, 70);
  int color = (int8_t)random(0, 64);
  Serial.print(t);
  Serial.print(" ");
  Serial.print(x);
  Serial.print(" ");
  Serial.print(y);
  Serial.print(" ");
  Serial.print(w);
  Serial.print(" ");
  Serial.print(h);
  Serial.print(" ");
  Serial.print(color);
  Serial.print(" ");
  int s, hi;
  if (t == 0) {
    lpm.drawLine(x, y, x + w, y + h, color);
    s = y;
    hi = h + 1;
  }
  else if (t == 1) {
    lpm.drawRect(x, y, w, h, color);
    s = y;
    hi = h;
  }
  else {
    lpm.drawEllipse(x, y, w, h, color);
    s = y - h - 1;
    hi = (h + 1) * 2;
  }
  unsigned long start = micros();
  lpm.flush(s, hi);
  unsigned long end = micros();
  Serial.print(" flush timeuse ");
  Serial.print(end - start);
  Serial.println(" us");
  delay(500);
}

int lpx = 0;
int lpy = 0;

void joystick()  {
  int ax, ay;
  ax = analogRead(A1);
  ay = analogRead(A0);
  Serial.print(ax);
  Serial.print(", ");
  Serial.println(ay);

  int px, py;
  px = 240 - int((1.0 * ax / 4096) * 240);
  py = int((1.0 * ay / 4096) * 240);

  // lpm.fill(0);
  lpm.drawRect(lpx - 5, lpy - 5, 10, 10, 0);
  lpm.drawRect(px - 5, py - 5, 10, 10, 63);
  int sy = min(py - 5, lpy - 5);
  int hi = max(lpy + 5 - sy, py + 5 - sy);
  lpm.flush(sy, hi);
  lpx = px;
  lpy = py;
  delay(40);
}

int cf = 0;
bool key_ls = true;

void loop() {
  if (cf == 0) allchars();
  else if (cf == 1) loading_anim();
  else if (cf == 2) joystick();
  else if (cf == 3) colors();
  else if (cf == 4) randgraphics();
  else cf = 0;
  int key_cs = digitalRead(key);
  if (key_cs == LOW && key_ls == HIGH) {
    cf = (cf + 1) % 5;
    lpm.fill(0);
    lpm.flush();
  }
  key_ls = key_cs;
}
