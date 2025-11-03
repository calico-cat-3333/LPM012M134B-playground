#ifndef _LPM012M134B_H_
#define _LPM012M134B_H_
#include <stdint.h>


// compressed 6-bit gray to 2-bit gray bayer dither LUT
// useage : (compressed_bayer_lut[6bitgray] >> (((x & 3) | ((y << 2) & 12)) << 1)) & 3
const uint32_t compressed_bayer_lut[64] = {
  0, 1, 1048577, 1048593, 1048593, 1114129, 1115153, 1074856977,
  1074856977, 1074873361, 1141982225, 1141982229, 1141982229, 1146176533, 1146176597, 1146438741,
  1146438741, 1146438997, 1414874453, 1414878549, 1414878549, 1431655765, 1431655766, 1432704342,
  1432704358, 1432704358, 1432769894, 1432770918, 2506512742, 2506512742, 2506529126, 2573637990,
  2573637994, 2573637994, 2577832298, 2577832362, 2578094506, 2578094506, 2578094762, 2846530218,
  2846534314, 2846534314, 2863311530, 2863311531, 2864360107, 2864360107, 2864360123, 2864425659,
  2864426683, 2864426683, 3938168507, 3938184891, 4005293755, 4005293755, 4005293759, 4009488063,
  4009488127, 4009488127, 4009750271, 4009750527, 4278185983, 4278185983, 4278190079, 4294967295,
};

// must include lvgl.h before include this file.
#ifndef LPM012M134B_USE_FRAMEBUFFER
  #ifdef LVGL_SRC_H
    #define LPM012M134B_USE_FRAMEBUFFER 0
  #else
    #define LPM012M134B_USE_FRAMEBUFFER 1
  #endif
#endif

class LPM012M134B {
  private:
    int xrst, vst, vck, enb, hst, hck, frp, xfrp;
    int r1, r2, g1, g2, b1, b2;
    #if LPM012M134B_USE_FRAMEBUFFER
    int8_t framebuffer[240][240];
    #endif
  public:
    int width = 240;
    int height = 240;
    LPM012M134B(int vst, int vck, int enb, int xrst, int frp, int xfrp, int hst, int hck,
                int r1, int r2, int g1, int g2, int b1, int b2);
    
    void init();

    #if LPM012M134B_USE_FRAMEBUFFER
    void flush(int rstart=0, int height=240);

    void fill(int8_t rgb222);

    void drawPixel(int x, int y, int8_t rgb222);

    void drawFastHLine(int x, int y, int w, int8_t rgb222);

    void drawFastVLine(int x, int y, int h, int8_t rgb222);

    void drawLine(int x0, int y0, int x1, int y1, int8_t rgb222);

    void drawRect(int x, int y, int w, int h, int8_t rgb222);

    void drawEllipse(int xc, int yc, int a, int b, int8_t rgb222);
    #endif // LPM012M134B_USE_FRAMEBUFFER

    void directflush_rgb565(int y1, int y2, uint16_t * buf);

    uint16_t quantize_rgb565_dithered(uint16_t rgb565, int x, int y);

};

#endif
