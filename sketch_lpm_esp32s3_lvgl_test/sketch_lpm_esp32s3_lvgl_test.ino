#include <lvgl.h>
#include "demos/lv_demos.h"
#include "examples/lv_examples.h"
#include "lpm012m134b.h"
#include "stdint.h"

#define ADC_KEYS 9
#define KEY_MENU 21
#define KEY_BACK 0

#define LCD_BL 46

LPM012M134B lpm(14, 13, 12, 11, 10, -1, 18, 17, 16, 15, 7, 6, 5, 4);

#define digitalToggle(pin) digitalWrite(pin, !digitalRead(pin))

#define TFT_HOR_RES   240
#define TFT_VER_RES   240
#define TFT_ROTATION  LV_DISPLAY_ROTATION_0

/*LVGL draw into this buffer, 1/10 screen size usually works well. The size is in bytes*/
#define DRAW_BUF_SIZE (TFT_HOR_RES * TFT_VER_RES / 4 * (LV_COLOR_DEPTH / 8))

alignas(LV_DRAW_BUF_ALIGN) uint8_t draw_buf1[DRAW_BUF_SIZE];
alignas(LV_DRAW_BUF_ALIGN) uint8_t draw_buf2[DRAW_BUF_SIZE];

#if LV_USE_LOG != 0
  void my_print( lv_log_level_t level, const char * buf ) {
      LV_UNUSED(level);
      Serial.println(buf);
      Serial.flush();
  }
#endif

bool use_bayer = true;
void my_disp_flush( lv_display_t *disp, const lv_area_t *area, uint8_t * px_map)
{
  uint16_t * buf16 = (uint16_t *)px_map;
  uint16_t * bufs = buf16;
  unsigned long start = micros();
  if (use_bayer) {
    lpm.bayer_dither_buffer(0, area->y1, 240, area->y2 - area->y1 + 1, buf16);
  }
  lpm.flush_buffer_rgb565(area->y1, area->y2, bufs);
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

lv_display_t * disp;

lv_group_t * g;
lv_indev_t * indev;

void btn_as_enc_read(lv_indev_t * indev, lv_indev_data_t * data){
  bool key_menu = digitalRead(KEY_MENU);
  bool key_back = digitalRead(KEY_BACK);
  uint16_t key_stat = analogRead(ADC_KEYS);
  if (key_stat > 4000 && key_back == true) {
    data->state = LV_INDEV_STATE_RELEASED;
  }
  else if (key_back == false) {
    printf("BACK\n");
    data->key = LV_KEY_ESC;
    data->state = LV_INDEV_STATE_PRESSED;
  }
  else if (key_stat <= 100) {
    printf("LEFT\n");
    if (key_menu == true) data->key = LV_KEY_LEFT;
    else data->key = LV_KEY_UP;
    data->state = LV_INDEV_STATE_PRESSED;
  }
  else if (key_stat >= 400 && key_stat <= 900) {
    printf("ENTER\n");
    data->key = LV_KEY_ENTER;
    data->state = LV_INDEV_STATE_PRESSED;
  }
  else if (key_stat >= 1100 && key_stat <= 1500) {
    printf("RIGHT\n");
    if (key_menu == true) data->key = LV_KEY_RIGHT;
    else data->key = LV_KEY_DOWN;
    data->state = LV_INDEV_STATE_PRESSED;
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  analogReadResolution(12);

  pinMode(LCD_BL, OUTPUT);
  digitalWrite(LCD_BL, HIGH);
  lpm.init();

  pinMode(KEY_BACK, INPUT_PULLUP);
  pinMode(KEY_MENU, INPUT_PULLUP);

  String LVGL_Arduino = "Hello Arduino!  LVGL:";
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

  g = lv_group_create();
  indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_ENCODER);
  lv_indev_set_mode(indev, LV_INDEV_MODE_TIMER);
  lv_indev_set_read_cb(indev, btn_as_enc_read);
  lv_group_set_default(g);
  lv_indev_set_group(indev, g);

  lv_demo_widgets();
  //lv_example_keyboard_1();
  //lv_demo_benchmark();

  Serial.println("Setup done");
}

// bool key_menu_lt = true;
// bool key_back_lt = true;

void loop() {
  // put your main code here, to run repeatedly:
  // bool key_menu_curr, key_back_curr;
  // key_menu_curr = digitalRead(KEY_MENU);
  // key_back_curr = digitalRead(KEY_BACK);

  // if (!key_back_curr && key_back_lt) {
  //   digitalToggle(LCD_BL);
  // }
  // if(!key_menu_curr && key_menu_lt) {
  //   use_bayer = !use_bayer;
  //   lv_obj_invalidate(lv_screen_active());
  // }
  // key_menu_lt = key_menu_curr;
  // key_back_lt = key_back_curr;
  lv_timer_handler(); /* let the GUI do its work */
  delay(5); /* let this time pass */
}
