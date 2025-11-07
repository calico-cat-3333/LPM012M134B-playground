#ifndef _LPM012M134B_H_
#define _LPM012M134B_H_
#include <stdint.h>


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
		static const uint32_t compressed_bayer_lut[64];
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

		void drawPixelRGB565(int x, int y, uint16_t rgb565);

		void drawFastHLineRGB565(int x, int y, int w, uint16_t rgb565);

		void drawFastVLineRGB565(int x, int y, int h, uint16_t rgb565);

		void drawLineRGB565(int x0, int y0, int x1, int y1, uint16_t rgb565);

		void drawRectRGB565(int x, int y, int w, int h, uint16_t rgb565);

		void drawEllipseRGB565(int xc, int yc, int a, int b, uint16_t rgb565);

		void fillRGB565(uint16_t rgb565);

		#endif // LPM012M134B_USE_FRAMEBUFFER

		void directflush_rgb565(int y1, int y2, uint16_t * buf);

		uint16_t quantize_rgb565_dithered(uint16_t rgb565, int x, int y);

		int8_t rgb565_to_rgb222(uint16_t rgb565);

};

#endif
