#include "pico/stdlib.h"
#include "hardware/pwm.h"

#define digitalToggle(pin) digitalWrite(pin, !digitalRead(pin))

// int xrst = 26;
// int vst = 0;
// int vck = 1;
// int enb = 2;
// int hst = 3;
// int hck = 4;
// int r1 = 6;
// int r2 = 7;
// int g1 = 8;
// int g2 = 9;
// int b1 = 10;
// int b2 = 11;
// int frp = 20;
// int xfrp = 21;

// rst pin 不只是为了 RESET 似乎还决定是否处于刷新模式（非刷新模式 RST 固定为低电平）
// void lpm_rst() {
//   digitalWrite(xrst, HIGH);
//   digitalWrite(xrst, LOW);
//   delayMicroseconds(163);
//   digitalWrite(xrst, HIGH);
//   delayMicroseconds(23);
// }

class LPM012M134B {
  private:
  int xrst, vst, vck, enb, hst, hck, frp, xfrp;
  int r1, r2, g1, g2, b1, b2;
  uint pwm_slice;
  int8_t framebuffer[240][240];
  public:
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

// void lpm_init() {
//   pinMode(xrst, OUTPUT);
//   pinMode(vst, OUTPUT);
//   pinMode(vck, OUTPUT);
//   pinMode(enb, OUTPUT);
//   pinMode(hst, OUTPUT);
//   pinMode(hck, OUTPUT);
//   pinMode(r1, OUTPUT);
//   pinMode(r2, OUTPUT);
//   pinMode(g1, OUTPUT);
//   pinMode(g2, OUTPUT);
//   pinMode(b1, OUTPUT);
//   pinMode(b2, OUTPUT);

//   gpio_set_function(frp, GPIO_FUNC_PWM);
//   gpio_set_function(xfrp, GPIO_FUNC_PWM);
//   uint slice = pwm_gpio_to_slice_num(frp);
//   pwm_set_wrap(slice, 8137);
//   pwm_set_chan_level(slice, PWM_CHAN_A, 4069);
//   pwm_set_chan_level(slice, PWM_CHAN_B, 4069);
//   pwm_set_output_polarity(slice, false, true);
//   pwm_set_clkdiv_int_frac(slice, 255, 0);
//   pwm_set_enabled(slice, true);

//   digitalWrite(xrst, LOW);
//   digitalWrite(vck, LOW);
//   digitalWrite(vst, LOW);
//   digitalWrite(hck, LOW);
//   digitalWrite(hst, LOW);
//   digitalWrite(enb, LOW);
//   digitalWrite(r1, LOW);
//   digitalWrite(r2, LOW);
//   digitalWrite(g1, LOW);
//   digitalWrite(g2, LOW);
//   digitalWrite(b1, LOW);
//   digitalWrite(b2, LOW);
// }

// void flush_old_fortestonly(bool r, bool g, bool b) {
//   // first version
//   // ref https://github.com/andelf/rp-embassy-playground/blob/master/src/lpm012m134b.rs
//   digitalWrite(vst, HIGH);
//   delayMicroseconds(40);
//   digitalToggle(vck);
//   delayMicroseconds(40);
//   digitalWrite(vst, LOW);
//   for (int i = 0; i < 487; i++) {
//     if (i >= 1 && i < 481) {
//       digitalWrite(hst, HIGH);
//       digitalToggle(hck);
//       digitalWrite(hst, LOW);
//       digitalWrite(enb, HIGH);
//       for (int j=0; j < 120; j++) {
//         if (j == 20) digitalWrite(enb, LOW);
//         digitalWrite(r1, r ? HIGH : LOW);
//         digitalWrite(g1, g ? HIGH : LOW);
//         digitalWrite(b1, b ? HIGH : LOW);
//         digitalWrite(r2, r ? HIGH : LOW);
//         digitalWrite(g2, g ? HIGH : LOW);
//         digitalWrite(b2, b ? HIGH : LOW);
//         delayMicroseconds(1);
//         digitalToggle(hck);
//       }
//       delayMicroseconds(1);
//       digitalToggle(hck);
//     }
//     else {
//       delayMicroseconds(1);
//     }
//     digitalToggle(vck);
//   }
// }

// int8_t framebuffer[240][240]

// // fast draw on rp2040
// // full flush timeuse: 190+ ms -> 66 ms
// #define digitalWrite gpio_put
// #undef digitalToggle
// #define digitalToggle(pin) gpio_put(pin, !gpio_get(pin))

// // timeuse 66 ms
// void flush_full() {
//   // full flush only
//   digitalWrite(xrst, HIGH);
//   delayMicroseconds(20);
//   digitalWrite(vst, HIGH);
//   delayMicroseconds(40);
//   digitalToggle(vck);
//   delayMicroseconds(40);
//   digitalWrite(vst, LOW);
//   digitalToggle(vck);
//   //delayMicroseconds(1);
//   for (int i = 0; i < 480; i++) {
//     digitalWrite(hst, HIGH);
//     digitalToggle(hck);
//     digitalWrite(hst, LOW);
//     digitalWrite(enb, HIGH);
//     for (int j = 0; j < 120; j++) {
//       if (j == 20) digitalWrite(enb, LOW);
//       int8_t cpixel = framebuffer[i / 2][j * 2];
//       int8_t npixel = framebuffer[i / 2][(j * 2) + 1];
//       if (i % 2 == 1) { // SPB
//         digitalWrite(r1, (cpixel & 0b010000) ? HIGH : LOW);
//         digitalWrite(g1, (cpixel & 0b000100) ? HIGH : LOW);
//         digitalWrite(b1, (cpixel & 0b000001) ? HIGH : LOW);
//         digitalWrite(r2, (npixel & 0b010000) ? HIGH : LOW);
//         digitalWrite(g2, (npixel & 0b000100) ? HIGH : LOW);
//         digitalWrite(b2, (npixel & 0b000001) ? HIGH : LOW);
//       }
//       else { // LPB
//         digitalWrite(r1, (cpixel & 0b100000) ? HIGH : LOW);
//         digitalWrite(g1, (cpixel & 0b001000) ? HIGH : LOW);
//         digitalWrite(b1, (cpixel & 0b000010) ? HIGH : LOW);
//         digitalWrite(r2, (npixel & 0b100000) ? HIGH : LOW);
//         digitalWrite(g2, (npixel & 0b001000) ? HIGH : LOW);
//         digitalWrite(b2, (npixel & 0b000010) ? HIGH : LOW);
//       }
//       //delayMicroseconds(1);
//       digitalToggle(hck);
//     }
//     //delayMicroseconds(1);
//     digitalToggle(vck);
//     digitalToggle(hck);
//   }
//   for (int i=0; i < 6; i++) {
//     if (i == 3) digitalWrite(xrst, LOW);
//     delayMicroseconds(1);
//     digitalToggle(vck);
//   }
// }

// // 下文中，所有的注释数字与数据手册时序图的对应均为对应数字前的边沿

// // full flush timeuse 63 ms
// void flush(int rstart=0, int height=240) {
//   // support partial (line) update
//   // start : start line index
//   // height : update area height
//   int start = max(0, rstart) * 2;
//   int end = min(240, height + start) * 2;
//   digitalWrite(xrst, HIGH); // xrst high, enter update mode
//   delayMicroseconds(20);
//   digitalWrite(vst, HIGH);
//   delayMicroseconds(40);
//   digitalToggle(vck); // vck 1
//   delayMicroseconds(40);
//   digitalWrite(vst, LOW);
//   digitalToggle(vck); // vck 2
//   //delayMicroseconds(1);
//   for (int i = 0; i < 486; i++) {
//     if (i >= start && i < end) {
//       digitalWrite(hst, HIGH);
//       digitalToggle(hck); // hck 1
//       digitalWrite(hst, LOW);
//       if (i != start) digitalWrite(enb, HIGH); // 第一个 enb 高电平实际发生在 LPB1 后
//       for (int j = 0; j < 120; j++) {
//         if (j == 20) digitalWrite(enb, LOW);
//         int8_t cpixel = framebuffer[i / 2][j * 2];
//         int8_t npixel = framebuffer[i / 2][(j * 2) + 1];
//         if (i % 2 == 1) { // SPB
//           digitalWrite(r1, (cpixel & 0b010000) ? HIGH : LOW);
//           digitalWrite(g1, (cpixel & 0b000100) ? HIGH : LOW);
//           digitalWrite(b1, (cpixel & 0b000001) ? HIGH : LOW);
//           digitalWrite(r2, (npixel & 0b010000) ? HIGH : LOW);
//           digitalWrite(g2, (npixel & 0b000100) ? HIGH : LOW);
//           digitalWrite(b2, (npixel & 0b000001) ? HIGH : LOW);
//         }
//         else { // LPB
//           digitalWrite(r1, (cpixel & 0b100000) ? HIGH : LOW);
//           digitalWrite(g1, (cpixel & 0b001000) ? HIGH : LOW);
//           digitalWrite(b1, (cpixel & 0b000010) ? HIGH : LOW);
//           digitalWrite(r2, (npixel & 0b100000) ? HIGH : LOW);
//           digitalWrite(g2, (npixel & 0b001000) ? HIGH : LOW);
//           digitalWrite(b2, (npixel & 0b000010) ? HIGH : LOW);
//         }
//         //delayMicroseconds(1);
//         digitalToggle(hck); // hck 2~121
//       }
//       //delayMicroseconds(1);
//       digitalToggle(vck); // vck 3~482 中的有效数据刷新部分
//       digitalToggle(hck); // hck 122
//     }
//     else {
//       if (i == end) {
//         digitalWrite(enb, HIGH); // 最后一个 enb 高电平发生在 SPB240 后
//         delayMicroseconds(40);
//         digitalWrite(enb, LOW);
//       }
//       if (i == 484) digitalWrite(xrst, LOW); // xrst low, exit update mode
//       delayMicroseconds(1);
//       digitalToggle(vck); // vck 3~488 中的无数据部分
//     }
//   }
// }

// // 65 ms
// void flush_vv(int rstart=0, int height=240) {
//   // support partial (line) update
//   // start : start line index
//   // height : update area height
//   int start = max(0, rstart) * 2;
//   int end = min(240, height + rstart) * 2;
//   digitalWrite(xrst, HIGH); // xrst high, enter update mode
//   delayMicroseconds(20);
//   digitalWrite(vst, HIGH); // vst high
//   delayMicroseconds(40);
//   digitalToggle(vck); // vck 1
//   delayMicroseconds(40);
//   digitalWrite(vst, LOW); // vst low
//   digitalToggle(vck); // vck 2
//   //delayMicroseconds(1);
//   for (int i = 0; i < start; i++) {
//     delayMicroseconds(1);
//     digitalToggle(vck); // 全刷模式无用，局刷模式表示有效数据前的 vck 信号
//   }
//   for (int i = start; i < end; i++) {
//     digitalWrite(hst, HIGH); // hst high
//     digitalToggle(hck); // hck 1
//     digitalWrite(hst, LOW); // hst low
//     if (i != start) digitalWrite(enb, HIGH); // enb high, enb 的第一个信号在 LPB1 之后
//     for (int j = 0; j < 120; j++) {
//       if (j == 20) digitalWrite(enb, LOW); // enb low
//       int8_t cpixel = framebuffer[i / 2][j * 2];
//       int8_t npixel = framebuffer[i / 2][(j * 2) + 1];
//       if (i % 2 == 1) { // SPB data
//         digitalWrite(r1, (cpixel & 0b010000) ? HIGH : LOW);
//         digitalWrite(g1, (cpixel & 0b000100) ? HIGH : LOW);
//         digitalWrite(b1, (cpixel & 0b000001) ? HIGH : LOW);
//         digitalWrite(r2, (npixel & 0b010000) ? HIGH : LOW);
//         digitalWrite(g2, (npixel & 0b000100) ? HIGH : LOW);
//         digitalWrite(b2, (npixel & 0b000001) ? HIGH : LOW);
//       }
//       else { // LPB data
//         digitalWrite(r1, (cpixel & 0b100000) ? HIGH : LOW);
//         digitalWrite(g1, (cpixel & 0b001000) ? HIGH : LOW);
//         digitalWrite(b1, (cpixel & 0b000010) ? HIGH : LOW);
//         digitalWrite(r2, (npixel & 0b100000) ? HIGH : LOW);
//         digitalWrite(g2, (npixel & 0b001000) ? HIGH : LOW);
//         digitalWrite(b2, (npixel & 0b000010) ? HIGH : LOW);
//       }
//       //delayMicroseconds(1);
//       digitalToggle(hck); // hck 2~121
//     }
//     delayMicroseconds(1);
//     digitalToggle(vck); // 全刷模式表示 vck 3~482, 局刷模式表示刷新区域的 vck 信号
//     digitalToggle(hck); // hck 122
//   }
//   digitalWrite(enb, HIGH); // enb high, enb 的最后一个信号在 SPB240 之后
//   delayMicroseconds(40);
//   digitalWrite(enb, LOW); // enb low
//   for (int i = end; i < 484; i++) {
//     delayMicroseconds(1);
//     digitalToggle(vck); // 全刷模式表示 vck 483~486，局刷模式表示有效区域后vck 486前的 vck 信号
//   }
//   digitalWrite(xrst, LOW); // xrst low, exit update mode
//   delayMicroseconds(1);
//   digitalToggle(vck); // vck 487
//   delayMicroseconds(1);
//   digitalToggle(vck); // vck 488
// }

// #undef digitalWrite
// #undef digitalToggle

LPM012M134B lpm(0, 1, 2, 26, 20, 21, 3, 4, 6, 7, 8, 9, 10, 11);

void setup() {
  // put your setup code here, to run once:
  Serial.begin();
  randomSeed(micros());
  // lpm_init();
  // lpm_rst();
  // fill(0);
  // flush();
  lpm.init();
}

int8_t c = 0;

void loop_colorflush() {
  // put your main code here, to run repeatedly:
  Serial.print(c);
  lpm.fill(c);
  unsigned long start = micros();
  lpm.flush();
  unsigned long end = micros();
  Serial.print(" flush timeuse ");
  Serial.print(end - start);
  Serial.println(" us");
  delay(1000);
  c = (c + 1) % 64;
}

void loop_randlines() {
  int sx = (int)random(0, 240);
  int sy = (int)random(0, 240);
  int ex = (int)random(0, 240);
  int ey = (int)random(0, 240);
  int color = (int8_t)random(0, 64);
  Serial.print(sx);
  Serial.print(" ");
  Serial.print(sy);
  Serial.print(" ");
  Serial.print(ex);
  Serial.print(" ");
  Serial.print(ey);
  Serial.print(" ");
  Serial.print(color);
  Serial.print(" ");
  lpm.drawLine(sx, sy, ex, ey, color);
  unsigned long start = micros();
  int dsy = min(sy, ey);
  int dhi = max(ey, sy) - dsy + 1;
  lpm.flush(dsy, dhi);
  unsigned long end = micros();
  Serial.print(" flush timeuse ");
  Serial.print(end - start);
  Serial.println(" us");
  delay(300);
}

void loop_randrects() {
  int x = (int)random(0, 190);
  int y = (int)random(0, 190);
  int w = (int)random(20, 70);
  int h = (int)random(20, 70);
  int color = (int8_t)random(0, 64);
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
  lpm.drawRect(x, y, w, h, color);
  unsigned long start = micros();
  lpm.flush();
  unsigned long end = micros();
  Serial.print(" flush timeuse ");
  Serial.print(end - start);
  Serial.println(" us");
  delay(500);
}

void loop() {
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