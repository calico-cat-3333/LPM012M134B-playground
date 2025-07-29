#include "pico/stdlib.h"
#include "hardware/pwm.h"
extern "C" {
  #include "ugui.h"
}

#define digitalToggle(pin) digitalWrite(pin, !digitalRead(pin))

class LPM012M134B {
  private:
  int xrst, vst, vck, enb, hst, hck, frp, xfrp;
  int r1, r2, g1, g2, b1, b2;
  uint pwm_slice;
  int8_t framebuffer[240][240];
  public:
  int width = 240;
  int height = 240;
  LPM012M134B(int vst, int vck, int enb, int xrst, int frp, int xfrp, int hst, int hck,
              int r1, int r2, int g1, int g2, int b1, int b2)
              : vst(vst), vck(vck), enb(enb), xrst(xrst), frp(frp), xfrp(xfrp), hst(hst), hck(hck),
                r1(r1), r2(r2), g1(g1), g2(g2), b1(b1), b2(b2) {}

  void init() {
    pinMode(this->xrst, OUTPUT);
    pinMode(this->vst, OUTPUT);
    pinMode(this->vck, OUTPUT);
    pinMode(this->enb, OUTPUT);
    pinMode(this->hst, OUTPUT);
    pinMode(this->hck, OUTPUT);
    pinMode(this->r1, OUTPUT);
    pinMode(this->r2, OUTPUT);
    pinMode(this->g1, OUTPUT);
    pinMode(this->g2, OUTPUT);
    pinMode(this->b1, OUTPUT);
    pinMode(this->b2, OUTPUT);

    gpio_set_function(this->frp, GPIO_FUNC_PWM);
    gpio_set_function(this->xfrp, GPIO_FUNC_PWM);
    if (pwm_gpio_to_slice_num(this->frp) != pwm_gpio_to_slice_num(this->xfrp)) {
      while (true) {
        Serial.println("Error: frp and xfrp in different slice!!!!");
        delay(1000 * 5);
      }
    }
    this->pwm_slice = pwm_gpio_to_slice_num(this->frp);
    pwm_set_wrap(this->pwm_slice, 8137);
    pwm_set_chan_level(this->pwm_slice, PWM_CHAN_A, 4069);
    pwm_set_chan_level(this->pwm_slice, PWM_CHAN_B, 4069);
    pwm_set_output_polarity(this->pwm_slice, false, true);
    pwm_set_clkdiv_int_frac(this->pwm_slice, 255, 0);
    pwm_set_enabled(this->pwm_slice, true);

    digitalWrite(this->xrst, LOW);
    digitalWrite(this->vck, LOW);
    digitalWrite(this->vst, LOW);
    digitalWrite(this->hck, LOW);
    digitalWrite(this->hst, LOW);
    digitalWrite(this->enb, LOW);
    digitalWrite(this->r1, LOW);
    digitalWrite(this->r2, LOW);
    digitalWrite(this->g1, LOW);
    digitalWrite(this->g2, LOW);
    digitalWrite(this->b1, LOW);
    digitalWrite(this->b2, LOW);
    this->fill(0);
    this->flush();
  }

#ifdef ARDUINO_ARCH_RP2040
  // fast draw on rp2040
  #define digitalWrite gpio_put
  #undef digitalToggle
  #define digitalToggle(pin) gpio_put(pin, !gpio_get(pin))
#endif
  void flush(int rstart=0, int height=240) {
    // support partial (line) update
    // start : start line index
    // height : update area height
    int start = max(0, rstart) * 2;
    int end = min(240, height + rstart) * 2;
    digitalWrite(xrst, HIGH); // xrst high, enter update mode
    delayMicroseconds(20);
    digitalWrite(vst, HIGH);
    delayMicroseconds(40);
    digitalToggle(vck); // vck 1
    delayMicroseconds(40);
    digitalWrite(vst, LOW);
    digitalToggle(vck); // vck 2
    //delayMicroseconds(1);
    for (int i = 0; i < 486; i++) {
      if (i >= start && i < end) {
        digitalWrite(hst, HIGH);
        digitalToggle(hck); // hck 1
        digitalWrite(hst, LOW);
        if (i != start) digitalWrite(enb, HIGH); // 第一个 enb 高电平实际发生在 LPB1 后
        for (int j = 0; j < 120; j++) {
          if (j == 20) digitalWrite(enb, LOW);
          int8_t cpixel = framebuffer[i / 2][j * 2];
          int8_t npixel = framebuffer[i / 2][(j * 2) + 1];
          if (i % 2 == 1) { // SPB
            digitalWrite(r1, (cpixel & 0b010000) ? HIGH : LOW);
            digitalWrite(g1, (cpixel & 0b000100) ? HIGH : LOW);
            digitalWrite(b1, (cpixel & 0b000001) ? HIGH : LOW);
            digitalWrite(r2, (npixel & 0b010000) ? HIGH : LOW);
            digitalWrite(g2, (npixel & 0b000100) ? HIGH : LOW);
            digitalWrite(b2, (npixel & 0b000001) ? HIGH : LOW);
          }
          else { // LPB
            digitalWrite(r1, (cpixel & 0b100000) ? HIGH : LOW);
            digitalWrite(g1, (cpixel & 0b001000) ? HIGH : LOW);
            digitalWrite(b1, (cpixel & 0b000010) ? HIGH : LOW);
            digitalWrite(r2, (npixel & 0b100000) ? HIGH : LOW);
            digitalWrite(g2, (npixel & 0b001000) ? HIGH : LOW);
            digitalWrite(b2, (npixel & 0b000010) ? HIGH : LOW);
          }
          //delayMicroseconds(1);
          digitalToggle(hck); // hck 2~121
        }
        //delayMicroseconds(1);
        digitalToggle(vck); // vck 3~482 中的有效数据刷新部分
        digitalToggle(hck); // hck 122
      }
      else {
        if (i == end) {
          digitalWrite(enb, HIGH); // 最后一个 enb 高电平发生在 SPB240 后
          delayMicroseconds(40);
          digitalWrite(enb, LOW);
        }
        if (i == 484) digitalWrite(xrst, LOW); // xrst low, exit update mode
        delayMicroseconds(1);
        digitalToggle(vck); // vck 3~488 中的无数据部分
      }
    }
  }

#ifdef ARDUINO_ARCH_RP2040
  // end fast draw on rp2040
  #undef digitalWrite
  #undef digitalToggle
  #define digitalToggle(pin) digitalWrite(pin, !digitalRead(pin))
#endif

  void fill(int8_t rgb222) {
    for (int i = 0; i < 240; i++) {
      for (int j = 0; j < 240; j++) {
        framebuffer[i][j] = rgb222;
      }
    }
  }

  // by chatgpt
  void setPixel(int x, int y, int8_t rgb222) {
    if (x < 0 || x >= 240 || y < 0 || y >= 240) return;
    framebuffer[y][x] = rgb222;
  }

  // by chatgpt
  void drawLine(int x0, int y0, int x1, int y1, uint8_t color) {
    int dx = abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy; // error value e_xy

    while (true) {
      if (!(x0 < 0 || x0 >= 240 || y0 < 0 || y0 >= 240)) framebuffer[y0][x0] = color;
      if (x0 == x1 && y0 == y1) break;
      int e2 = 2 * err;
      if (e2 >= dy) {
        err += dy;
        x0 += sx;
      }
      if (e2 <= dx) {
        err += dx;
        y0 += sy;
      }
    }
  }

  // by chatgpt
  void drawRect(int x, int y, int w, int h, int color) {
    for (int j = y; j < y + h; j++) {
      if (j < 0 || j >= 240) continue;
      for (int i = x; i < x + w; i++) {
        if (i < 0 || i >= 240) continue;
        framebuffer[j][i] = color;
      }
    }
  }

  // by chatgpt
  void drawEllipse(int xc, int yc, int a, int b, int color) {
    for (int y = -b; y <= b; y++) {
      for (int x = -a; x <= a; x++) {
        // 标准椭圆方程：(x² / a²) + (y² / b²) <= 1
        if ((x * x) * (b * b) + (y * y) * (a * a) <= (a * a) * (b * b)) {
          int px = xc + x;
          int py = yc + y;
          if (px >= 0 && px < 240 && py >= 0 && py < 240) {
            framebuffer[py][px] = color;
          }
        }
      }
    }
  }
};

LPM012M134B lpm(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13);
UG_GUI gui;

int ymin = 240;
int ymax = 0;

void pset(UG_S16 x, UG_S16 y, UG_COLOR c) {
  uint8_t rgb222c = ((c & 0xc00000) >> 18) | ((c & 0x00c000) >> 12) | ((c & 0x0000c0) >> 6);
  lpm.setPixel(x, y, rgb222c);
  ymin = min(ymin, y);
  ymax = max(ymax, y);
}

int bl = 14;
int key = 24;

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
    hi = h * 2;
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
  ax = analogRead(A0);
  ay = analogRead(A1);
  Serial.print(ax);
  Serial.print(", ");
  Serial.println(ay);

  int px, py;
  px = 240 - int((1.0 * ax / 4096) * 240);
  py = 240 - int((1.0 * ay / 4096) * 240);

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
