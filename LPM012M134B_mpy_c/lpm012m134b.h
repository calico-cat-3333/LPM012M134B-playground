#ifndef _LPM012M134B_H_
#define _LPM012M134B_H_

// #ifdef __cplusplus
// extern "C" {
// #endif

#include "py/obj.h"
#include "py/runtime.h"

#define WHITE 0b111111
#define BLACK 0b000000
#define RED 0b110000
#define GREEN 0b001100
#define BLUE 0b000011
#define YELLOW 0b111100
#define CYAN 0b001111
#define MAGENTA 0b110011

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

typedef struct _lpm012m134b_LPM012M134B_obj_t {
	mp_obj_base_t base;

	uint16_t width;
	uint16_t height;
	uint8_t * framebuffer;
	bool use_fb;
	mp_hal_pin_obj_t xrst, vst, vck, enb, hst, hck, frp, xfrp;
	mp_hal_pin_obj_t r1, r2, g1, g2, b1, b2;
	mp_hal_pin_obj_t cs;
	mp_hal_pin_obj_t backlight;

} lpm012m134b_LPM012M134B_obj_t;

mp_obj_t lpm012m134b_LPM012M134B_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args);


// #ifdef  __cplusplus
// }
// #endif //  __cplusplus

#endif // _LPM012M134B_H_