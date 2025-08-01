#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include <lvgl.h>
#include "demos/lv_demos.h"
#include "examples/lv_examples.h"


#define digitalToggle(pin) digitalWrite(pin, !digitalRead(pin))

    // Bayer 4x4
const uint8_t bayer[4][4] = {
  { 0,  8,  2, 10 },
  {12,  4, 14,  6 },
  { 3, 11,  1,  9 },
  {15,  7, 13,  5 }
};

// 6-bit gray to 2-bit gray bayer dither LUT
const uint8_t bayer_lut[64][4][4] = {
{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
{{1, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
{{1, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 0}},
{{1, 0, 1, 0}, {0, 0, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 0}},
{{1, 0, 1, 0}, {0, 0, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 0}},
{{1, 0, 1, 0}, {0, 0, 0, 0}, {1, 0, 1, 0}, {0, 0, 0, 0}},
{{1, 0, 1, 0}, {0, 1, 0, 0}, {1, 0, 1, 0}, {0, 0, 0, 0}},
{{1, 0, 1, 0}, {0, 1, 0, 0}, {1, 0, 1, 0}, {0, 0, 0, 1}},
{{1, 0, 1, 0}, {0, 1, 0, 0}, {1, 0, 1, 0}, {0, 0, 0, 1}},
{{1, 0, 1, 0}, {0, 1, 0, 1}, {1, 0, 1, 0}, {0, 0, 0, 1}},
{{1, 0, 1, 0}, {0, 1, 0, 1}, {1, 0, 1, 0}, {0, 1, 0, 1}},
{{1, 1, 1, 0}, {0, 1, 0, 1}, {1, 0, 1, 0}, {0, 1, 0, 1}},
{{1, 1, 1, 0}, {0, 1, 0, 1}, {1, 0, 1, 0}, {0, 1, 0, 1}},
{{1, 1, 1, 0}, {0, 1, 0, 1}, {1, 0, 1, 1}, {0, 1, 0, 1}},
{{1, 1, 1, 1}, {0, 1, 0, 1}, {1, 0, 1, 1}, {0, 1, 0, 1}},
{{1, 1, 1, 1}, {0, 1, 0, 1}, {1, 1, 1, 1}, {0, 1, 0, 1}},
{{1, 1, 1, 1}, {0, 1, 0, 1}, {1, 1, 1, 1}, {0, 1, 0, 1}},
{{1, 1, 1, 1}, {1, 1, 0, 1}, {1, 1, 1, 1}, {0, 1, 0, 1}},
{{1, 1, 1, 1}, {1, 1, 0, 1}, {1, 1, 1, 1}, {0, 1, 1, 1}},
{{1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 1, 1}, {0, 1, 1, 1}},
{{1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 1, 1}, {0, 1, 1, 1}},
{{1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 1, 1}},
{{2, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 1, 1}},
{{2, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 2, 1}, {1, 1, 1, 1}},
{{2, 1, 2, 1}, {1, 1, 1, 1}, {1, 1, 2, 1}, {1, 1, 1, 1}},
{{2, 1, 2, 1}, {1, 1, 1, 1}, {1, 1, 2, 1}, {1, 1, 1, 1}},
{{2, 1, 2, 1}, {1, 1, 1, 1}, {2, 1, 2, 1}, {1, 1, 1, 1}},
{{2, 1, 2, 1}, {1, 2, 1, 1}, {2, 1, 2, 1}, {1, 1, 1, 1}},
{{2, 1, 2, 1}, {1, 2, 1, 1}, {2, 1, 2, 1}, {1, 1, 1, 2}},
{{2, 1, 2, 1}, {1, 2, 1, 1}, {2, 1, 2, 1}, {1, 1, 1, 2}},
{{2, 1, 2, 1}, {1, 2, 1, 2}, {2, 1, 2, 1}, {1, 1, 1, 2}},
{{2, 1, 2, 1}, {1, 2, 1, 2}, {2, 1, 2, 1}, {1, 2, 1, 2}},
{{2, 2, 2, 1}, {1, 2, 1, 2}, {2, 1, 2, 1}, {1, 2, 1, 2}},
{{2, 2, 2, 1}, {1, 2, 1, 2}, {2, 1, 2, 1}, {1, 2, 1, 2}},
{{2, 2, 2, 1}, {1, 2, 1, 2}, {2, 1, 2, 2}, {1, 2, 1, 2}},
{{2, 2, 2, 2}, {1, 2, 1, 2}, {2, 1, 2, 2}, {1, 2, 1, 2}},
{{2, 2, 2, 2}, {1, 2, 1, 2}, {2, 2, 2, 2}, {1, 2, 1, 2}},
{{2, 2, 2, 2}, {1, 2, 1, 2}, {2, 2, 2, 2}, {1, 2, 1, 2}},
{{2, 2, 2, 2}, {2, 2, 1, 2}, {2, 2, 2, 2}, {1, 2, 1, 2}},
{{2, 2, 2, 2}, {2, 2, 1, 2}, {2, 2, 2, 2}, {1, 2, 2, 2}},
{{2, 2, 2, 2}, {2, 2, 2, 2}, {2, 2, 2, 2}, {1, 2, 2, 2}},
{{2, 2, 2, 2}, {2, 2, 2, 2}, {2, 2, 2, 2}, {1, 2, 2, 2}},
{{2, 2, 2, 2}, {2, 2, 2, 2}, {2, 2, 2, 2}, {2, 2, 2, 2}},
{{3, 2, 2, 2}, {2, 2, 2, 2}, {2, 2, 2, 2}, {2, 2, 2, 2}},
{{3, 2, 2, 2}, {2, 2, 2, 2}, {2, 2, 3, 2}, {2, 2, 2, 2}},
{{3, 2, 2, 2}, {2, 2, 2, 2}, {2, 2, 3, 2}, {2, 2, 2, 2}},
{{3, 2, 3, 2}, {2, 2, 2, 2}, {2, 2, 3, 2}, {2, 2, 2, 2}},
{{3, 2, 3, 2}, {2, 2, 2, 2}, {3, 2, 3, 2}, {2, 2, 2, 2}},
{{3, 2, 3, 2}, {2, 3, 2, 2}, {3, 2, 3, 2}, {2, 2, 2, 2}},
{{3, 2, 3, 2}, {2, 3, 2, 2}, {3, 2, 3, 2}, {2, 2, 2, 2}},
{{3, 2, 3, 2}, {2, 3, 2, 2}, {3, 2, 3, 2}, {2, 2, 2, 3}},
{{3, 2, 3, 2}, {2, 3, 2, 3}, {3, 2, 3, 2}, {2, 2, 2, 3}},
{{3, 2, 3, 2}, {2, 3, 2, 3}, {3, 2, 3, 2}, {2, 3, 2, 3}},
{{3, 2, 3, 2}, {2, 3, 2, 3}, {3, 2, 3, 2}, {2, 3, 2, 3}},
{{3, 3, 3, 2}, {2, 3, 2, 3}, {3, 2, 3, 2}, {2, 3, 2, 3}},
{{3, 3, 3, 2}, {2, 3, 2, 3}, {3, 2, 3, 3}, {2, 3, 2, 3}},
{{3, 3, 3, 3}, {2, 3, 2, 3}, {3, 2, 3, 3}, {2, 3, 2, 3}},
{{3, 3, 3, 3}, {2, 3, 2, 3}, {3, 2, 3, 3}, {2, 3, 2, 3}},
{{3, 3, 3, 3}, {2, 3, 2, 3}, {3, 3, 3, 3}, {2, 3, 2, 3}},
{{3, 3, 3, 3}, {3, 3, 2, 3}, {3, 3, 3, 3}, {2, 3, 2, 3}},
{{3, 3, 3, 3}, {3, 3, 2, 3}, {3, 3, 3, 3}, {2, 3, 3, 3}},
{{3, 3, 3, 3}, {3, 3, 2, 3}, {3, 3, 3, 3}, {2, 3, 3, 3}},
{{3, 3, 3, 3}, {3, 3, 3, 3}, {3, 3, 3, 3}, {2, 3, 3, 3}},
{{3, 3, 3, 3}, {3, 3, 3, 3}, {3, 3, 3, 3}, {3, 3, 3, 3}},
};


class LPM012M134B {
  private:
  int xrst, vst, vck, enb, hst, hck, frp, xfrp;
  int r1, r2, g1, g2, b1, b2;
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

    analogWriteFreq(100);
    analogWriteRange(256);
    analogWrite(this->frp, 127);

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
    //this->fill(0);
    //this->flush();
  }

#ifdef ARDUINO_ARCH_RP2040
  // fast draw on rp2040
  #define digitalWrite gpio_put
  #undef digitalToggle
  #define digitalToggle(pin) gpio_put(pin, !gpio_get(pin))
#endif

  void directflush_rgb565(int y1, int y2, uint16_t * buf) {
    // flush a rgb565 buffer, for lvgl display flush callback
    // support partial (line) update
    // y1: lvgl area->y1
    // y2: lvgl area->y2
    int start = max(0, y1) * 2;
    int end = min(240, y2 + 1) * 2;
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
          uint16_t * pixelpointer = buf + (240 * ((i - start) / 2)) + j * 2;
          uint16_t cpixel, npixel;
          cpixel = *pixelpointer;
          npixel = *(pixelpointer + 1);
          if (i % 2 == 1) { // SPB
            digitalWriteFast(r1, (cpixel & 0x4000)); // (1 << 14)
            digitalWriteFast(g1, (cpixel & 0x0200)); // (1 << 9)
            digitalWriteFast(b1, (cpixel & 0x0008)); // (1 << 3)
            digitalWriteFast(r2, (npixel & 0x4000));
            digitalWriteFast(g2, (npixel & 0x0200));
            digitalWriteFast(b2, (npixel & 0x0008));
          }
          else { // LPB
            digitalWriteFast(r1, (cpixel & 0x8000)); // (1 << 15)
            digitalWriteFast(g1, (cpixel & 0x0400)); // (1 << 10)
            digitalWriteFast(b1, (cpixel & 0x0010)); // (1 << 4)
            digitalWriteFast(r2, (npixel & 0x8000));
            digitalWriteFast(g2, (npixel & 0x0400));
            digitalWriteFast(b2, (npixel & 0x0010));
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


  // by chatgpt with my modification
  uint16_t quantize_rgb565_dithered(uint16_t rgb565, int x, int y) {
    // 取 RGB565 各通道
    // uint8_t r = ((rgb565 >> 11) & 0x1F) << 3;
    // uint8_t g = ((rgb565 >> 5) & 0x3F) << 2;
    // uint8_t b = (rgb565 & 0x1F) << 3;
    // uint8_t threshold = bayer[y & 3][x & 3] * 16; // % 4 == & 0b11 == & 3

    // // uint8_t r2 = ctest(r, threshold);
    // // uint8_t g2 = ctest(g, threshold);
    // // uint8_t b2 = ctest(b, threshold);
    // uint8_t r2 = min(3, r / 85 + int(r % 85 > threshold / 3));
    // uint8_t g2 = min(3, g / 85 + int(g % 85 > threshold / 3));
    // uint8_t b2 = min(3, b / 85 + int(b % 85 > threshold / 3));
    uint8_t r2 = bayer_lut[((rgb565 >> 11) & 0x1F) << 1][y & 3][x & 3];
    uint8_t g2 = bayer_lut[((rgb565 >> 5) & 0x3F)][y & 3][x & 3];
    uint8_t b2 = bayer_lut[(rgb565 & 0x1F) << 1][y & 3][x & 3];
    return (r2 << 14) | (g2 << 9) | (b2 << 3);
    // return (r2 << 4) | (g2 << 2) | b2;
  }

};

// 传输刷新块信息
// 用于双核刷新加速
// 核心 2 负责刷新，核心 1 负责将刷新范围发送到第二核心。
// 利用 fifo 进行发送，第二核心接收 rfa1 地址，并刷新，第二核心空闲时使用 fifo 向第一核心发送空闲标志。
struct refresh_area {
  uint16_t *buf;
  int y1;
  int y2;
} rfa1;
refresh_area *prfa1 = & rfa1;

LPM012M134B lpm(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13);

#define TFT_HOR_RES   240
#define TFT_VER_RES   240
#define TFT_ROTATION  LV_DISPLAY_ROTATION_0

/*LVGL draw into this buffer, 1/10 screen size usually works well. The size is in bytes*/
#define DRAW_BUF_SIZE (TFT_HOR_RES * TFT_VER_RES / 4 * (LV_COLOR_DEPTH / 8))
uint8_t draw_buf1[DRAW_BUF_SIZE];
uint8_t draw_buf2[DRAW_BUF_SIZE];

#if LV_USE_LOG != 0
void my_print( lv_log_level_t level, const char * buf )
{
    LV_UNUSED(level);
    Serial.println(buf);
    Serial.flush();
}
#endif

bool use_bayer = true;
void my_disp_flush( lv_display_t *disp, const lv_area_t *area, uint8_t * px_map)
{
  rfa1.y1 = area->y1;
  rfa1.y2 = area->y2;
  rfa1.buf = (uint16_t *)px_map;
  rp2040.fifo.push(1);
  // int t = rp2040.fifo.pop();
  // Serial.print("Core1: flush timeuse ");
  // Serial.print(t);
  // Serial.println(" us");
}
// void my_disp_flush( lv_display_t *disp, const lv_area_t *area, uint8_t * px_map)
// {
//   uint16_t * buf16 = (uint16_t *)px_map;
//   uint16_t * bufs = buf16;
//   unsigned long start = micros();
//   if (use_bayer) {
//     for (int i = area->y1; i <= area->y2; i++) {
//       for (int j = area->x1; j <= area->x2; j++) {
//         *buf16 = lpm.quantize_rgb565_dithered(*buf16, j, i);
//         buf16 ++;
//       }
//     }
//   }
//   lpm.directflush_rgb565(area->y1, area->y2, bufs);
//   unsigned long end = micros();
//   Serial.print("flush timeuse ");
//   Serial.print(end - start);
//   Serial.println(" us");
//   lv_display_flush_ready(disp);
// }

/*Read the joystick*/
// int posx = 120;
// int posy = 120;
// void my_joystick_read( lv_indev_t * indev, lv_indev_data_t * data )
// {
//   int ax, ay;
//   ax = analogRead(A0);
//   ay = analogRead(A1);
//   int px, py;
//   px = int(ax > 2500 || ax < 1500) * ((2048 - ax) / 100);
//   py = int(ay > 2500 || ay < 1500) * ((2048 - ay) / 100);

//   bool ntouched = (bool)digitalRead(24);
//   posx = max(0, min(240, posx + px));
//   posy = max(0, min(240, posy + py));
//   data->point.x = posx;
//   data->point.y = posy;
//   if(ntouched) {
//     data->state = LV_INDEV_STATE_RELEASED;
//   } else {
//     data->state = LV_INDEV_STATE_PRESSED;
//   }
// }
void my_joystick_read( lv_indev_t * indev, lv_indev_data_t * data )
{
  int ax, ay;
  ay = analogRead(A0);
  ax = analogRead(A1);
  // Serial.print(ax);
  // Serial.print(' ');
  // Serial.println(ay);
  int px, py;
  px = 240 - int((1.0 * min(ax, 4096) / 4096) * 240);
  py = int((1.0 * min(ay, 4096) / 4096) * 240);

  bool ntouched = bool(digitalRead(24)) and bool(digitalRead(15)) and bool(digitalRead(22));
  data->point.x = px;
  data->point.y = py;
  if(ntouched) {
    data->state = LV_INDEV_STATE_RELEASED;
  } else {
    data->state = LV_INDEV_STATE_PRESSED;
  }
}

/*use Arduinos millis() as tick source*/
static uint32_t my_tick(void)
{
    return millis();
}

void rounder_event_cb(lv_event_t * e)
{
  lv_area_t * a = lv_event_get_invalidated_area(e);

  a->x1 = 0;
  a->x2 = TFT_HOR_RES - 1;
}

int bl = 14;
lv_display_t * disp;

void setup() {
  Serial.begin();

  pinMode(24, INPUT_PULLUP);
  analogReadResolution(12);

  String LVGL_Arduino = "Hello Arduino! ";
  LVGL_Arduino += String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();
  Serial.println(LVGL_Arduino);

  lv_init();
  lv_tick_set_cb(my_tick);

#if LV_USE_LOG != 0
  lv_log_register_print_cb(my_print);
#endif
  disp = lv_display_create(TFT_HOR_RES, TFT_VER_RES);
  lv_display_set_flush_cb(disp, my_disp_flush);
  lv_display_set_buffers(disp, draw_buf1, draw_buf2, sizeof(draw_buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);
  lv_display_add_event_cb(disp, rounder_event_cb, LV_EVENT_INVALIDATE_AREA, NULL);

  lv_indev_t * mouse_indev = lv_indev_create();
  lv_indev_set_type(mouse_indev, LV_INDEV_TYPE_POINTER); /*Touchpad should have POINTER type*/
  lv_indev_set_read_cb(mouse_indev, my_joystick_read);
  lv_obj_t * cursor_obj = lv_image_create(lv_screen_active());  /* Create image Widget for cursor. */
  lv_image_set_src(cursor_obj, LV_SYMBOL_CLOSE);             /* Set image source. */
  lv_indev_set_cursor(mouse_indev, cursor_obj);

  lv_demo_widgets();
  //lv_example_image_2();
  //lv_demo_benchmark();

  Serial.println("Setup done");
}

bool key2_lt = true;
bool key3_lt = true;

void loop() {
  bool c20, c21;
  c20 = digitalRead(20);
  c21 = digitalRead(21);
  if (!c21 && key2_lt) {
    digitalToggle(bl);
  }
  if(!c20 && key3_lt) {
    use_bayer = !use_bayer;
    lv_obj_invalidate(lv_screen_active());
  }
  key2_lt = c21;
  key3_lt = c20;
  // put your main code here, to run repeatedly:
  lv_timer_handler(); /* let the GUI do its work */
  delay(5); /* let this time pass */
}

bool core1_separate_stack = true;

void setup1() {
  delay(500);
  pinMode(bl, OUTPUT);
  digitalWrite(bl, HIGH);
  lpm.init();
}

void loop1(){
  rp2040.fifo.pop();
  uint16_t * buf16 = (*prfa1).buf;
  uint16_t * bufs = buf16;
  int y1 = (*prfa1).y1;
  int y2 = (*prfa1).y2;
  // unsigned long start = micros();
  if (use_bayer) {
    for (int i = y1; i <= y2; i++) {
      for (int j = 0; j <= 239; j++) {
        *buf16 = lpm.quantize_rgb565_dithered(*buf16, j, i);
        buf16 ++;
      }
    }
  }
  lpm.directflush_rgb565(y1, y2, bufs);
  // unsigned long end = micros();
  // rp2040.fifo.push(end - start);
  lv_display_flush_ready(disp);
}