#include "lpm012m134b.h"
#include "Arduino.h"

// compressed 6-bit gray to 2-bit gray bayer dither LUT
// useage : (compressed_bayer_lut[6bitgray] >> (((x & 3) | ((y << 2) & 12)) << 1)) & 3
const uint32_t LPM012M134B::compressed_bayer_lut[64] = {
	0, 1, 1048577, 1048593, 1048593, 1114129, 1115153, 1074856977,
	1074856977, 1074873361, 1141982225, 1141982229, 1141982229, 1146176533, 1146176597, 1146438741,
	1146438741, 1146438997, 1414874453, 1414878549, 1414878549, 1431655765, 1431655766, 1432704342,
	1432704358, 1432704358, 1432769894, 1432770918, 2506512742, 2506512742, 2506529126, 2573637990,
	2573637994, 2573637994, 2577832298, 2577832362, 2578094506, 2578094506, 2578094762, 2846530218,
	2846534314, 2846534314, 2863311530, 2863311531, 2864360107, 2864360107, 2864360123, 2864425659,
	2864426683, 2864426683, 3938168507, 3938184891, 4005293755, 4005293755, 4005293759, 4009488063,
	4009488127, 4009488127, 4009750271, 4009750527, 4278185983, 4278185983, 4278190079, 4294967295,
};

LPM012M134B::LPM012M134B(int vst, int vck, int enb, int xrst, int frp, int xfrp,
int hst, int hck, int r1, int r2, int g1, int g2, int b1, int b2):
vst(vst), vck(vck), enb(enb), xrst(xrst), frp(frp), xfrp(xfrp), hst(hst), hck(hck),
r1(r1), r2(r2), g1(g1), g2(g2), b1(b1), b2(b2) {}

void LPM012M134B::init() {
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

#ifndef digitalWriteFast
	#define digitalWriteFast(pin, val) digitalWrite(pin, val ? HIGH : LOW)
	#define _DWF_CUSTOM 1
#endif

#ifndef digitalToggle
	#ifdef ARDUINO_ARCH_RP2040
		#define digitalToggle(pin) gpio_put(pin, !gpio_get(pin))
	#else
		#define digitalToggle(pin) digitalWrite(pin, !digitalRead(pin))
	#endif
	#define _DT_CUSTOM 1
#endif

void LPM012M134B::directflush_rgb565(int y1, int y2, uint16_t * buf) {
	// flush a rgb565 buffer, for lvgl display flush callback
	// support partial (line) update
	// y1: lvgl area->y1
	// y2: lvgl area->y2
	uint16_t * pixelpointer = buf;
	uint16_t cpixel, npixel;
	int i, j;
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
	for (i = 0; i < 486; i++) {
		if (i >= start && i < end) {
			digitalWrite(hst, HIGH);
			digitalToggle(hck); // hck 1
			digitalWrite(hst, LOW);
			if (i != start) digitalWrite(enb, HIGH); // 第一个 enb 高电平实际发生在 LPB1 后
			for (j = 0; j < 120; j++) {
				if (j == 20) digitalWrite(enb, LOW);
				//pixelpointer = buf + (240 * ((i - start) / 2)) + j * 2;
				cpixel = *pixelpointer;
				npixel = *(pixelpointer + 1);
				pixelpointer = pixelpointer + 2;
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
					if (j == 119) pixelpointer = pixelpointer - 240; // LPB done, then SPB
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

#if LPM012M134B_USE_FRAMEBUFFER
void LPM012M134B::flush(int rstart, int height) {
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
					digitalWriteFast(r1, (cpixel & 0b010000));
					digitalWriteFast(g1, (cpixel & 0b000100));
					digitalWriteFast(b1, (cpixel & 0b000001));
					digitalWriteFast(r2, (npixel & 0b010000));
					digitalWriteFast(g2, (npixel & 0b000100));
					digitalWriteFast(b2, (npixel & 0b000001));
				}
				else { // LPB
					digitalWriteFast(r1, (cpixel & 0b100000));
					digitalWriteFast(g1, (cpixel & 0b001000));
					digitalWriteFast(b1, (cpixel & 0b000010));
					digitalWriteFast(r2, (npixel & 0b100000));
					digitalWriteFast(g2, (npixel & 0b001000));
					digitalWriteFast(b2, (npixel & 0b000010));
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

void LPM012M134B::fill(int8_t rgb222) {
	// for (int i = 0; i < 240; i++) {
	// 	for (int j = 0; j < 240; j++) {
	// 		framebuffer[i][j] = rgb222;
	// 	}
	// }
	memset(framebuffer, rgb222, 240 * 240);
}

void LPM012M134B::drawPixel(int x, int y, int8_t rgb222) {
	if (x < 0 || x >= 240 || y < 0 || y >= 240) return;
	framebuffer[y][x] = rgb222;
}

void LPM012M134B::drawFastHLine(int x, int y, int w, int8_t rgb222) {
	if (y < 0 || y >= 240) return;
	int sx = max(x, 0);
	int safew = min(x + w, 240) - sx;
	memset(&framebuffer[y][sx], rgb222, safew);
}

void LPM012M134B::drawFastVLine(int x, int y, int h, int8_t rgb222) {
	if (x < 0 || x >= 240) return;
	int endv = min(y + h, 240);
	for(int i = max(y, 0); i < endv; i++) {
		framebuffer[i][x] = rgb222;
	}
}

// by chatgpt
void LPM012M134B::drawLine(int x0, int y0, int x1, int y1, int8_t rgb222) {
	int dx = abs(x1 - x0);
	int sx = x0 < x1 ? 1 : -1;
	int dy = -abs(y1 - y0);
	int sy = y0 < y1 ? 1 : -1;
	int err = dx + dy; // error value e_xy

	while (true) {
		if (!(x0 < 0 || x0 >= 240 || y0 < 0 || y0 >= 240)) framebuffer[y0][x0] = rgb222;
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

void LPM012M134B::drawRect(int x, int y, int w, int h, int8_t rgb222) {
	int endv = min(y + h, 240);
	for (int i = max(y, 0); i < endv; i++) {
		drawFastHLine(x, i, w, rgb222);
	}
}

// by chatgpt
void LPM012M134B::drawEllipse(int xc, int yc, int a, int b, int8_t rgb222) {
	for (int y = -b; y <= b; y++) {
		for (int x = -a; x <= a; x++) {
			// 标准椭圆方程：(x² / a²) + (y² / b²) <= 1
			if ((x * x) * (b * b) + (y * y) * (a * a) <= (a * a) * (b * b)) {
				int px = xc + x;
				int py = yc + y;
				if (px >= 0 && px < 240 && py >= 0 && py < 240) {
					framebuffer[py][px] = rgb222;
				}
			}
		}
	}
}

// draw RGB565 colord shape with bayer dither
void LPM012M134B::drawPixelRGB565(int x, int y, uint16_t rgb565) {
	drawPixel(x, y, rgb565_to_rgb222(quantize_rgb565_dithered(rgb565, x, y)));
}

void LPM012M134B::drawFastHLineRGB565(int x, int y, int w, uint16_t rgb565) {
	if (y < 0 || y >= 240) return;
	int endv = min(x + w, 240);
	for(int i = max(x, 0); i < endv; i++) {
		framebuffer[y][i] = rgb565_to_rgb222(quantize_rgb565_dithered(rgb565, i, y));
	}
}

void LPM012M134B::drawFastVLineRGB565(int x, int y, int h, uint16_t rgb565) {
	if (x < 0 || x >= 240) return;
	int endv = min(y + h, 240);
	for(int i = max(y, 0); i < endv; i++) {
		framebuffer[i][x] = rgb565_to_rgb222(quantize_rgb565_dithered(rgb565, x, i));
	}
}

void LPM012M134B::drawLineRGB565(int x0, int y0, int x1, int y1, uint16_t rgb565) {
	int dx = abs(x1 - x0);
	int sx = x0 < x1 ? 1 : -1;
	int dy = -abs(y1 - y0);
	int sy = y0 < y1 ? 1 : -1;
	int err = dx + dy; // error value e_xy

	while (true) {
		if (!(x0 < 0 || x0 >= 240 || y0 < 0 || y0 >= 240)) {
			framebuffer[y0][x0] = rgb565_to_rgb222(quantize_rgb565_dithered(rgb565, x0, y0));
		}
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

void LPM012M134B::drawRectRGB565(int x, int y, int w, int h, uint16_t rgb565) {
	int endv = min(y + h, 240);
	for (int i = max(y, 0); i < endv; i++) {
		drawFastHLineRGB565(x, i, w, rgb565);
	}
}

void LPM012M134B::drawEllipseRGB565(int xc, int yc, int a, int b, uint16_t rgb565) {
	for (int y = -b; y <= b; y++) {
		for (int x = -a; x <= a; x++) {
			// 标准椭圆方程：(x² / a²) + (y² / b²) <= 1
			if ((x * x) * (b * b) + (y * y) * (a * a) <= (a * a) * (b * b)) {
				int px = xc + x;
				int py = yc + y;
				if (px >= 0 && px < 240 && py >= 0 && py < 240) {
					framebuffer[py][px] = rgb565_to_rgb222(quantize_rgb565_dithered(rgb565, px, py));
				}
			}
		}
	}
}

void LPM012M134B::fillRGB565(uint16_t rgb565) {
	drawRectRGB565(0, 0, 240, 240, rgb565);
}

#endif //LPM012134B_USE_FRAMEBUFFER

#if _DWF_CUSTOM
	#undef digitalWriteFast
#endif

#if _DT_CUSTOM
	#undef digitalToggle
#endif

uint16_t LPM012M134B::quantize_rgb565_dithered(uint16_t rgb565, int x, int y) {
	uint8_t r2 = (compressed_bayer_lut[((rgb565 >> 11) & 0x1F) << 1] >> (((x & 3) | ((y << 2) & 12)) << 1)) & 3;
	uint8_t g2 = (compressed_bayer_lut[((rgb565 >> 5) & 0x3F)] >> (((x & 3) | ((y << 2) & 12)) << 1)) & 3;
	uint8_t b2 = (compressed_bayer_lut[(rgb565 & 0x1F) << 1] >> (((x & 3) | ((y << 2) & 12)) << 1)) & 3;
	return (r2 << 14) | (g2 << 9) | (b2 << 3);
	// return (r2 << 4) | (g2 << 2) | b2;
}

int8_t LPM012M134B::rgb565_to_rgb222(uint16_t rgb565) {
	return ((rgb565 >> 10) & 0b110000) | ((rgb565 >> 7) & 0b001100) | ((rgb565 >> 3) & 0b000011);
}
