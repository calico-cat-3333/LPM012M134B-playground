#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include <lvgl.h>
#include "demos/lv_demos.h"
#include "examples/lv_examples.h"
#include "lpm012m134b.h"

#define digitalToggle(pin) digitalWrite(pin, !digitalRead(pin))

// 打印刷新函数的耗时，注意会导致无法双缓冲刷新。
#define PRINT_TIMEUSE 0
// 是否使用核心 1 执行屏幕刷新
#define USE_CORE1_FLUSH 1
// 摇杆判定移动阈值
#define JOYSITCK_READ_THRESHOLD 20

// 传输刷新块信息
// 用于双核刷新加速
// 核心 2 负责刷新，核心 1 负责告知核心 2 开始刷新。
// 利用 fifo 为空时 pop 阻塞特性实现，利用指针 prfa1 传递刷新区域地址。
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
// 指定缓冲区对齐，在 RP2350 开启双核刷新上是必须的。
alignas(LV_DRAW_BUF_ALIGN) uint8_t draw_buf1[DRAW_BUF_SIZE];
alignas(LV_DRAW_BUF_ALIGN) uint8_t draw_buf2[DRAW_BUF_SIZE];

#if LV_USE_LOG != 0
void my_print( lv_log_level_t level, const char * buf )
{
    LV_UNUSED(level);
    Serial.println(buf);
    Serial.flush();
}
#endif

bool use_bayer = true;
#if USE_CORE1_FLUSH
void my_disp_flush( lv_display_t *disp, const lv_area_t *area, uint8_t * px_map)
{
  rfa1.y1 = area->y1;
  rfa1.y2 = area->y2;
  rfa1.buf = (uint16_t *)px_map;
  rp2040.fifo.push(1);
#if PRINT_TIMEUSE
  int t = rp2040.fifo.pop();
  Serial.print("Core1: flush timeuse ");
  Serial.print(t);
  Serial.print(" us for ");
  Serial.print(area->y2 - area->y1 + 1);
  Serial.println(" lines");
#endif // PRINT_TIMEUSE
}
#else
void my_disp_flush( lv_display_t *disp, const lv_area_t *area, uint8_t * px_map)
{
  uint16_t * buf16 = (uint16_t *)px_map;
  uint16_t * bufs = buf16;
  unsigned long start = micros();
  if (use_bayer) {
    for (int i = area->y1; i <= area->y2; i++) {
      for (int j = area->x1; j <= area->x2; j++) {
        *buf16 = lpm.quantize_rgb565_dithered(*buf16, j, i);
        buf16 ++;
      }
    }
  }
  lpm.directflush_rgb565(area->y1, area->y2, bufs);
  unsigned long end = micros();
#if PRINT_TIMEUSE
  Serial.print("Core0: flush timeuse ");
  Serial.print(end - start);
  Serial.print(" us for ");
  Serial.print(area->y2 - area->y1 + 1);
  Serial.println(" lines");
#endif // PRINT_TIMEUSE
  lv_display_flush_ready(disp);
}
#endif // USE_CORE1_FLUSH

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

int lax, lay;

void my_joystick_read( lv_indev_t * indev, lv_indev_data_t * data )
{
  int ax, ay;
  ay = analogRead(A0);
  ax = analogRead(A1);
  // Serial.print(ax);
  // Serial.print(' ');
  // Serial.print(ay);
  // Serial.print(' ');
  if (abs(lax - ax) <= JOYSITCK_READ_THRESHOLD) ax = lax; else lax = ax;
  if (abs(lay - ay) <= JOYSITCK_READ_THRESHOLD) ay = lay; else lay = ay;
  // Serial.print(ax);
  // Serial.print(' ');
  // Serial.println(ay);
  int px, py;
  px = 240 - int((1.0 * min(ax, 4096) / 4096) * 240);
  py = int((1.0 * min(ay, 4096) / 4096) * 240);

  bool ntouched = bool(digitalRead(15)) and bool(digitalRead(22));
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

  //pinMode(24, INPUT);
  pinMode(20, INPUT);
  pinMode(21, INPUT);
  pinMode(15, INPUT);
  pinMode(22, INPUT);
  analogReadResolution(12);

#if USE_CORE1_FLUSH != 1
  pinMode(bl, OUTPUT);
  digitalWrite(bl, HIGH);
  lpm.init();
#endif // USE_CORE_FLUSH != 1

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
  //lv_example_scale_6();
  //lv_demo_benchmark();

  Serial.println("Core0: Setup done");
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

#if USE_CORE1_FLUSH

bool core1_separate_stack = true;

void setup1() {
  pinMode(bl, OUTPUT);
  digitalWrite(bl, HIGH);
  lpm.init();
  Serial.println("Core1: Setup done");
}

void loop1(){
  rp2040.fifo.pop();
  uint16_t * buf16 = (*prfa1).buf;
  uint16_t * bufs = buf16;
  int y1 = (*prfa1).y1;
  int y2 = (*prfa1).y2;
#if PRINT_TIMEUSE
  unsigned long start = micros();
#endif // PRINT_TIMEUSE
  if (use_bayer) {
    for (int i = y1; i <= y2; i++) {
      for (int j = 0; j <= 239; j++) {
        *buf16 = lpm.quantize_rgb565_dithered(*buf16, j, i);
        buf16 ++;
      }
    }
  }
  lpm.directflush_rgb565(y1, y2, bufs);
#if PRINT_TIMEUSE
  unsigned long end = micros();
  rp2040.fifo.push(end - start);
#endif // PRINT_TIMEUSE
  lv_display_flush_ready(disp);
}
#endif // USE_CORE1_FLUSH