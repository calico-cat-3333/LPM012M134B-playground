#include "py/obj.h"
#include "py/runtime.h"
#include "mphalport.h"
#include <string.h>
#include "lpm012m134b.h"

static mp_obj_t lpm012m134b_color222(mp_obj_t r_obj, mp_obj_t g_obj, mp_obj_t b_obj) {
	int r = (mp_obj_get_int(r_obj) >> 2) & 0b110000;
	int g = (mp_obj_get_int(g_obj) >> 4) & 0b001100;
	int b = (mp_obj_get_int(b_obj) >> 6) & 0b000011;
	return mp_obj_new_int(r | g | b);
}
static MP_DEFINE_CONST_FUN_OBJ_3(lpm012m134b_color222_obj, lpm012m134b_color222);

static inline uint16_t color565to222(uint16_t rgb565) {	
	return ((rgb565 >> 10) & 0b110000) | ((rgb565 >> 7) & 0b001100) | ((rgb565 >> 3) & 0b000011);
}

static mp_obj_t lpm012m134b_color565to222(mp_obj_t color565_obj) {
	uint16_t rgb565 = mp_obj_get_int(color565_obj);
	return mp_obj_new_int(color565to222(rgb565));
}
static MP_DEFINE_CONST_FUN_OBJ_1(lpm012m134b_color565to222_obj, lpm012m134b_color565to222);

static inline uint16_t bayer_dither_point(int x, int y, uint16_t rgb565) {
	uint8_t r2 = (compressed_bayer_lut[((rgb565 >> 11) & 0x1F) << 1] >> (((x & 3) | ((y << 2) & 12)) << 1)) & 3;
	uint8_t g2 = (compressed_bayer_lut[((rgb565 >> 5) & 0x3F)] >> (((x & 3) | ((y << 2) & 12)) << 1)) & 3;
	uint8_t b2 = (compressed_bayer_lut[(rgb565 & 0x1F) << 1] >> (((x & 3) | ((y << 2) & 12)) << 1)) & 3;
	return (r2 << 14) | (g2 << 9) | (b2 << 3);	
}

static mp_obj_t lpm012m134b_bayer_dither_point(mp_obj_t x_obj, mp_obj_t y_obj, mp_obj_t rgb565_obj) {
	int rgb565 = mp_obj_get_int(rgb565_obj);
	int x = mp_obj_get_int(x_obj);
	int y = mp_obj_get_int(y_obj);
	return mp_obj_new_int(bayer_dither_point(x, y, rgb565));
}
static MP_DEFINE_CONST_FUN_OBJ_3(lpm012m134b_bayer_dither_point_obj, lpm012m134b_bayer_dither_point);

static inline void bayer_dither_buffer(int x, int y, int w, int h, uint16_t * buf) {
	for (int i = y; i < y + h; i++) {
		for (int j = x; j < x + w; j++) {
			*buf = bayer_dither_point(j, i, *buf);
			buf++;
		}
	}
}

static mp_obj_t lpm012m134b_bayer_dither_buffer(size_t n_args, const mp_obj_t *args_in) {
	int x = mp_obj_get_int(args_in[0]);
	int y = mp_obj_get_int(args_in[1]);
	int w = mp_obj_get_int(args_in[2]);
	int h = mp_obj_get_int(args_in[3]);
	
	mp_buffer_info_t bufinfo;
	mp_get_buffer_raise(args_in[4], &bufinfo, MP_BUFFER_RW);
	uint16_t *buf = (uint16_t *)bufinfo.buf;
	bayer_dither_buffer(x, y, w, h, buf);

	return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(lpm012m134b_bayer_dither_buffer_obj, 5, 5, lpm012m134b_bayer_dither_buffer);

static void lpm012m134b_LPM012M134B_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
	(void)kind;
	lpm012m134b_LPM012M134B_obj_t *self = MP_OBJ_TO_PTR(self_in);
	mp_printf(print, "<LPM012M134B, width=%d, height=%d>", self->width, self->height);
}

static mp_obj_t lpm012m134b_LPM012M134B_init(mp_obj_t self_in) {
	lpm012m134b_LPM012M134B_obj_t *self = MP_OBJ_TO_PTR(self_in);

	// 列出所有 Pin 字段
	mp_hal_pin_obj_t pins[] = {
		self->xrst, self->vst, self->vck, self->enb, self->hst, self->hck,
		self->r1, self->r2, self->g1, self->g2, self->b1, self->b2,
	};

	size_t count = sizeof(pins) / sizeof(pins[0]);

	// 初始化全为输出并写低电平
	for (size_t i = 0; i < count; i++) {
		mp_hal_pin_output(pins[i]);
		mp_hal_pin_write(pins[i], 0);
	}

	return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(lpm012m134b_LPM012M134B_init_obj, lpm012m134b_LPM012M134B_init);

#define MP_PIN_TOGGLE(pin) mp_hal_pin_write((pin), !(mp_hal_pin_read(pin)))
static mp_obj_t lpm012m134b_LPM012M134B_flush(size_t n_args, const mp_obj_t *args_in) {
	// support partial (line) update
	// start : start line index
	// height : update area height
	lpm012m134b_LPM012M134B_obj_t *self = MP_OBJ_TO_PTR(args_in[0]);
	if (self->use_fb == false) {
		mp_raise_ValueError("Framebuffer is disabled, cannot flush screen!");
	}
	int rstart = 0;
	int height = self->height;
	if (n_args == 2) {
		rstart = mp_obj_get_int(args_in[1]);
		height = self->height - rstart;
	}
	if (n_args == 3) {
		rstart = mp_obj_get_int(args_in[1]);
		height = mp_obj_get_int(args_in[2]);
	}
	int start = MAX(0, rstart) * 2;
	int end = MIN(240, height + rstart) * 2;
	mp_hal_pin_write(self->xrst, 1); // xrst high, enter update mode
	mp_hal_delay_us_fast(20);
	mp_hal_pin_write(self->vst, 1);
	mp_hal_delay_us_fast(40);
	MP_PIN_TOGGLE(self->vck); // vck 1
	mp_hal_delay_us_fast(40);
	mp_hal_pin_write(self->vst, 0);
	MP_PIN_TOGGLE(self->vck); // vck 2
	//mp_hal_delay_us_fast(1);
	for (int i = 0; i < 486; i++) {
		if (i >= start && i < end) {
			mp_hal_pin_write(self->hst, 1);
			MP_PIN_TOGGLE(self->hck); // hck 1
			mp_hal_pin_write(self->hst, 0);
			if (i != start) mp_hal_pin_write(self->enb, 1); // 第一个 enb 高电平实际发生在 LPB1 后
			for (int j = 0; j < 120; j++) {
				if (j == 20) mp_hal_pin_write(self->enb, 0);
				uint8_t cpixel = self->framebuffer[((i / 2) * self->width) + (j * 2)];
				uint8_t npixel = self->framebuffer[((i / 2) * self->width) + (j * 2) + 1];
				if (i % 2 == 1) { // SPB
					mp_hal_pin_write(self->r1, !!(cpixel & 0b010000));
					mp_hal_pin_write(self->g1, !!(cpixel & 0b000100));
					mp_hal_pin_write(self->b1, !!(cpixel & 0b000001));
					mp_hal_pin_write(self->r2, !!(npixel & 0b010000));
					mp_hal_pin_write(self->g2, !!(npixel & 0b000100));
					mp_hal_pin_write(self->b2, !!(npixel & 0b000001));
				}
				else { // LPB
					mp_hal_pin_write(self->r1, !!(cpixel & 0b100000));
					mp_hal_pin_write(self->g1, !!(cpixel & 0b001000));
					mp_hal_pin_write(self->b1, !!(cpixel & 0b000010));
					mp_hal_pin_write(self->r2, !!(npixel & 0b100000));
					mp_hal_pin_write(self->g2, !!(npixel & 0b001000));
					mp_hal_pin_write(self->b2, !!(npixel & 0b000010));
				}
				//mp_hal_delay_us_fast(1);
				MP_PIN_TOGGLE(self->hck); // hck 2~121
			}
			//mp_hal_delay_us_fast(1);
			MP_PIN_TOGGLE(self->vck); // vck 3~482 中的有效数据刷新部分
			MP_PIN_TOGGLE(self->hck); // hck 122
		}
		else {
			if (i == end) {
				mp_hal_pin_write(self->enb, 1); // 最后一个 enb 高电平发生在 SPB240 后
				mp_hal_delay_us_fast(40);
				mp_hal_pin_write(self->enb, 0);
			}
			if (i == 484) mp_hal_pin_write(self->xrst, 0); // xrst low, exit update mode
			mp_hal_delay_us_fast(1);
			MP_PIN_TOGGLE(self->vck); // vck 3~488 中的无数据部分
		}
	}
	return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(lpm012m134b_LPM012M134B_flush_obj, 1, 3, lpm012m134b_LPM012M134B_flush);

static mp_obj_t lpm012m134b_LPM012M134B_flush_buffer_rgb565(size_t n_args, const mp_obj_t *args_in) {
	// flush a rgb565 buffer, for lvgl display flush callback
	// support partial (line) update
	// y1: lvgl area->y1
	// y2: lvgl area->y2
	lpm012m134b_LPM012M134B_obj_t *self = MP_OBJ_TO_PTR(args_in[0]);
	int y1 = mp_obj_get_int(args_in[1]);
	int y2 = mp_obj_get_int(args_in[2]);

	mp_buffer_info_t bufinfo;
	mp_get_buffer_raise(args_in[3], &bufinfo, MP_BUFFER_READ);
	uint16_t *pixelpointer = (uint16_t *)bufinfo.buf;
	int cnt = 0;

	int start = MAX(0, y1) * 2;
	int end = MIN(240, y2 + 1) * 2;
	mp_hal_pin_write(self->xrst, 1); // xrst high, enter update mode
	mp_hal_delay_us_fast(20);
	mp_hal_pin_write(self->vst, 1);
	mp_hal_delay_us_fast(40);
	MP_PIN_TOGGLE(self->vck); // vck 1
	mp_hal_delay_us_fast(40);
	mp_hal_pin_write(self->vst, 0);
	MP_PIN_TOGGLE(self->vck); // vck 2
	//mp_hal_delay_us_fast(1);
	for (int i = 0; i < 486; i++) {
		if (i >= start && i < end) {
			mp_hal_pin_write(self->hst, 1);
			MP_PIN_TOGGLE(self->hck); // hck 1
			mp_hal_pin_write(self->hst, 0);
			if (i != start) mp_hal_pin_write(self->enb, 1); // 第一个 enb 高电平实际发生在 LPB1 后
			for (int j = 0; j < 120; j++) {
				if (j == 20) mp_hal_pin_write(self->enb, 0);
				uint16_t cpixel = pixelpointer[cnt];
				uint16_t npixel = pixelpointer[cnt + 1];
				cnt = cnt + 2;
				if (i % 2 == 1) { // SPB
					mp_hal_pin_write(self->r1, !!(cpixel & 0x4000));
					mp_hal_pin_write(self->g1, !!(cpixel & 0x0200));
					mp_hal_pin_write(self->b1, !!(cpixel & 0x0008));
					mp_hal_pin_write(self->r2, !!(npixel & 0x4000));
					mp_hal_pin_write(self->g2, !!(npixel & 0x0200));
					mp_hal_pin_write(self->b2, !!(npixel & 0x0008));
				}
				else { // LPB
					mp_hal_pin_write(self->r1, !!(cpixel & 0x8000));
					mp_hal_pin_write(self->g1, !!(cpixel & 0x0400));
					mp_hal_pin_write(self->b1, !!(cpixel & 0x0010));
					mp_hal_pin_write(self->r2, !!(npixel & 0x8000));
					mp_hal_pin_write(self->g2, !!(npixel & 0x0400));
					mp_hal_pin_write(self->b2, !!(npixel & 0x0010));
					if (j == 119) cnt = cnt - 240;
				}
				//mp_hal_delay_us_fast(1);
				MP_PIN_TOGGLE(self->hck); // hck 2~121
			}
			//mp_hal_delay_us_fast(1);
			MP_PIN_TOGGLE(self->vck); // vck 3~482 中的有效数据刷新部分
			MP_PIN_TOGGLE(self->hck); // hck 122
		}
		else {
			if (i == end) {
				mp_hal_pin_write(self->enb, 1); // 最后一个 enb 高电平发生在 SPB240 后
				mp_hal_delay_us_fast(40);
				mp_hal_pin_write(self->enb, 0);
			}
			if (i == 484) mp_hal_pin_write(self->xrst, 0); // xrst low, exit update mode
			mp_hal_delay_us_fast(1);
			MP_PIN_TOGGLE(self->vck); // vck 3~488 中的无数据部分
		}
	}
	return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(lpm012m134b_LPM012M134B_flush_buffer_rgb565_obj, 4, 4, lpm012m134b_LPM012M134B_flush_buffer_rgb565);

static mp_obj_t lpm012m134b_LPM012M134B_blit_buffer(size_t n_args, const mp_obj_t *args_in) {
	lpm012m134b_LPM012M134B_obj_t *self = MP_OBJ_TO_PTR(args_in[0]);
	mp_buffer_info_t bufinfo;
	mp_get_buffer_raise(args_in[1], &bufinfo, MP_BUFFER_READ);
	uint8_t *source = (uint8_t *)bufinfo.buf;
	int x = mp_obj_get_int(args_in[2]);
	int y = mp_obj_get_int(args_in[3]);
	int h = mp_obj_get_int(args_in[4]);
	int w = mp_obj_get_int(args_in[5]);
	for (int i = y; i < y + h; i++) {
		for (int j = x; j < x + w; j++) {
			self->framebuffer[i * self->width + j] = *source;
			source++;
		}
	}
	return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(lpm012m134b_LPM012M134B_blit_buffer_obj, 6, 6, lpm012m134b_LPM012M134B_blit_buffer);

static mp_obj_t lpm012m134b_LPM012M134B_blit_buffer_rgb565(size_t n_args, const mp_obj_t *args_in) {
	lpm012m134b_LPM012M134B_obj_t *self = MP_OBJ_TO_PTR(args_in[0]);
	mp_buffer_info_t bufinfo;
	mp_get_buffer_raise(args_in[1], &bufinfo, MP_BUFFER_READ);
	uint16_t *source = (uint16_t *)bufinfo.buf;
	int x = mp_obj_get_int(args_in[2]);
	int y = mp_obj_get_int(args_in[3]);
	int h = mp_obj_get_int(args_in[4]);
	int w = mp_obj_get_int(args_in[5]);
	bool use_bayer = false;
	if (n_args == 7) {
		use_bayer = mp_obj_is_true(args_in[6]);
	}
	for (int i = y; i < y + h; i++) {
		for (int j = x; j < x + w; j++) {
			self->framebuffer[i * self->width + j] = color565to222((use_bayer ? bayer_dither_point(j, i, *source) : *source));
			source++;
		}
	}
	return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(lpm012m134b_LPM012M134B_blit_buffer_rgb565_obj, 6, 7, lpm012m134b_LPM012M134B_blit_buffer_rgb565);

static const mp_rom_map_elem_t lpm012m134b_LPM012M134B_locals_dict_table[] = {
	{MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&lpm012m134b_LPM012M134B_init_obj)},
	{MP_ROM_QSTR(MP_QSTR_flush), MP_ROM_PTR(&lpm012m134b_LPM012M134B_flush_obj)},
	{MP_ROM_QSTR(MP_QSTR_flush_buffer_rgb565), MP_ROM_PTR(&lpm012m134b_LPM012M134B_flush_buffer_rgb565_obj)},
	{MP_ROM_QSTR(MP_QSTR_blit_buffer), MP_ROM_PTR(&lpm012m134b_LPM012M134B_blit_buffer_obj)},
	{MP_ROM_QSTR(MP_QSTR_blit_buffer_rgb565), MP_ROM_PTR(&lpm012m134b_LPM012M134B_blit_buffer_rgb565_obj)},
};
static MP_DEFINE_CONST_DICT(lpm012m134b_LPM012M134B_locals_dict, lpm012m134b_LPM012M134B_locals_dict_table);
/* methods end */

#ifdef MP_OBJ_TYPE_GET_SLOT

MP_DEFINE_CONST_OBJ_TYPE(
	lpm012m134b_LPM012M134B_type,
	MP_QSTR_LPM012M134B,
	MP_TYPE_FLAG_NONE,
	print, lpm012m134b_LPM012M134B_print,
	make_new, lpm012m134b_LPM012M134B_make_new,
	locals_dict, (mp_obj_dict_t *)&lpm012m134b_LPM012M134B_locals_dict);

#else

const mp_obj_type_t lpm012m134b_LPM012M134B_type = {
	{&mp_type_type},
	.name = MP_QSTR_LPM012M134B,
	.print = lpm012m134b_LPM012M134B_print,
	.make_new = lpm012m134b_LPM012M134B_make_new,
	.locals_dict = (mp_obj_dict_t *)&lpm012m134b_LPM012M134B_locals_dict,
};

#endif


mp_obj_t lpm012m134b_LPM012M134B_make_new(const mp_obj_type_t *type,
        size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
	enum {
		ARG_fbuf,
		ARG_vst, ARG_vck, ARG_enb, ARG_xrst, ARG_hst, ARG_hck,
		ARG_r1, ARG_r2, ARG_g1, ARG_g2, ARG_b1, ARG_b2,
	};

	static const mp_arg_t allowed_args[] = {
		{ MP_QSTR_fbuf, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },

		{ MP_QSTR_vst, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
		{ MP_QSTR_vck, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
		{ MP_QSTR_enb, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
		{ MP_QSTR_xrst, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },

		{ MP_QSTR_hst, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
		{ MP_QSTR_hck, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },

		{ MP_QSTR_r1, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
		{ MP_QSTR_r2, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
		{ MP_QSTR_g1, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
		{ MP_QSTR_g2, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
		{ MP_QSTR_b1, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
		{ MP_QSTR_b2, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
	};

	mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
	mp_arg_parse_all_kw_array(
		n_args, n_kw, all_args,
		MP_ARRAY_SIZE(allowed_args), allowed_args, args
	);

	// allocate object
	lpm012m134b_LPM012M134B_obj_t *self = m_new_obj(lpm012m134b_LPM012M134B_obj_t);
	self->base.type = type;

	// basic info
	self->width = 240;
	self->height = 240;

	// ----------------------------------------------------------
	// 1. framebuffer：bytearray → raw pointer
	// ----------------------------------------------------------
	if (args[ARG_fbuf].u_obj != mp_const_none) {
		mp_buffer_info_t bufinfo;
		mp_get_buffer_raise(args[ARG_fbuf].u_obj, &bufinfo, MP_BUFFER_RW);
		self->framebuffer = (uint8_t *)bufinfo.buf;
		self->use_fb = true;
	}
	else {
		self->framebuffer = NULL;
		self->use_fb = false;
	}

	// ----------------------------------------------------------
	// 2. Convert machine.Pin → mp_hal_pin_obj_t
	// ----------------------------------------------------------
	self->vst  = mp_hal_get_pin_obj(args[ARG_vst].u_obj);
	self->vck  = mp_hal_get_pin_obj(args[ARG_vck].u_obj);
	self->enb  = mp_hal_get_pin_obj(args[ARG_enb].u_obj);
	self->xrst = mp_hal_get_pin_obj(args[ARG_xrst].u_obj);

	self->hst  = mp_hal_get_pin_obj(args[ARG_hst].u_obj);
	self->hck  = mp_hal_get_pin_obj(args[ARG_hck].u_obj);

	self->r1 = mp_hal_get_pin_obj(args[ARG_r1].u_obj);
	self->r2 = mp_hal_get_pin_obj(args[ARG_r2].u_obj);
	self->g1 = mp_hal_get_pin_obj(args[ARG_g1].u_obj);
	self->g2 = mp_hal_get_pin_obj(args[ARG_g2].u_obj);
	self->b1 = mp_hal_get_pin_obj(args[ARG_b1].u_obj);
	self->b2 = mp_hal_get_pin_obj(args[ARG_b2].u_obj);

	return MP_OBJ_FROM_PTR(self);
}

static const mp_map_elem_t lpm012m134b_module_globals_table[] = {
	{MP_ROM_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_lpm012m134b)},
	{MP_ROM_QSTR(MP_QSTR_color222), (mp_obj_t)&lpm012m134b_color222_obj},
	{MP_ROM_QSTR(MP_QSTR_color565to222), (mp_obj_t)&lpm012m134b_color565to222_obj},
	{MP_ROM_QSTR(MP_QSTR_bayer_dither_point), (mp_obj_t)&lpm012m134b_bayer_dither_point_obj},
	{MP_ROM_QSTR(MP_QSTR_bayer_dither_buffer), (mp_obj_t)&lpm012m134b_bayer_dither_buffer_obj},
	{MP_ROM_QSTR(MP_QSTR_LPM012M134B), (mp_obj_t)&lpm012m134b_LPM012M134B_type},
	{MP_ROM_QSTR(MP_QSTR_BLACK), MP_ROM_INT(BLACK)},
	{MP_ROM_QSTR(MP_QSTR_BLUE), MP_ROM_INT(BLUE)},
	{MP_ROM_QSTR(MP_QSTR_RED), MP_ROM_INT(RED)},
	{MP_ROM_QSTR(MP_QSTR_GREEN), MP_ROM_INT(GREEN)},
	{MP_ROM_QSTR(MP_QSTR_CYAN), MP_ROM_INT(CYAN)},
	{MP_ROM_QSTR(MP_QSTR_MAGENTA), MP_ROM_INT(MAGENTA)},
	{MP_ROM_QSTR(MP_QSTR_YELLOW), MP_ROM_INT(YELLOW)},
	{MP_ROM_QSTR(MP_QSTR_WHITE), MP_ROM_INT(WHITE)},
};

static MP_DEFINE_CONST_DICT(mp_module_lpm012m134b_globals, lpm012m134b_module_globals_table);

const mp_obj_module_t mp_module_lpm012m134b = {
	.base = {&mp_type_module},
	.globals = (mp_obj_dict_t *)&mp_module_lpm012m134b_globals,
};

#if !defined(MICROPY_VERSION) || MICROPY_VERSION <= 70144
MP_REGISTER_MODULE(MP_QSTR_lpm012m134b, mp_module_lpm012m134b, MODULE_lpm012m134b_ENABLE);
#else
MP_REGISTER_MODULE(MP_QSTR_lpm012m134b, mp_module_lpm012m134b);
#endif
