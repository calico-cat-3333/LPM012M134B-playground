"""RP2040-Touch-LCD-1.28
"""

import lpm012m134b
import lvgl as lv
from machine import PWM, Pin
import time
import framebuf

class LPM012M134B_lv:
    def __init__(self, vst, vck, enb, xrst, frp, xfrp, hst, hck, r1, r2, g1, g2, b1, b2, bl, doublebuffer=False, factor=4):
        self.width = 240
        self.height = 240
        self.factor = factor
        self.doublebuffer = doublebuffer

        self.frp_pin = frp
        self.xfrp_pin = xfrp # no use
        self.bl_pin = bl
        self.vst = vst
        self.vck = vck
        self.enb = enb
        self.xrst = xrst
        self.hst = hst
        self.hck = hck
        self.r1 = r1
        self.r2 = r2
        self.g1 = g1
        self.g2 = g2
        self.b1 = b1
        self.b2 = b2
        

        self.frp = PWM(self.frp_pin)
        self.frp.freq(100)
        self.frp.duty_u16(32767)

        if bl is not None:
            self.backlight = PWM(bl)
            self.brightness = 100
            self.backlight.freq(5000)
            self.backlight.duty_u16(65535)

        self.tft_drv = lpm012m134b.LPM012M134B(None,
                                               vst, vck, enb, xrst, hst, hck,
                                               r1, r2, g1, g2, b1, b2)
        self.tft_drv.init()
    
    def after_lvgl_init(self):
        if not lv.is_initialized():
            lv.init()
        self.color_format = lv.COLOR_FORMAT.RGB565
        self.pixel_size = lv.color_format_get_size(self.color_format)

        self.draw_buf1 = lv.draw_buf_create(self.width, self.height // self.factor, self.color_format, 0)
        self.draw_buf2 = lv.draw_buf_create(self.width, self.height // self.factor, self.color_format, 0) if self.doublebuffer else None

        self.disp_drv = lv.display_create(self.width, self.height)
        self.disp_drv.set_color_format(self.color_format)
        self.disp_drv.set_draw_buffers(self.draw_buf1, self.draw_buf2)
        self.disp_drv.set_render_mode(lv.DISPLAY_RENDER_MODE.PARTIAL)
        self.disp_drv.set_flush_cb(self.tft_flush_cb)
        self.disp_drv.add_event_cb(self.round_area_cb, lv.EVENT.INVALIDATE_AREA, None)

    def tft_flush_cb(self, disp_drv, area, color_p):
        w = area.x2 - area.x1 + 1
        h = area.y2 - area.y1 + 1
        size = w * h
        data_view = color_p.__dereference__(size * self.pixel_size)
        lv.draw_sw_rgb565_swap(data_view, size)
        lpm012m134b.bayer_dither_buffer(0, area.y1, w, h, data_view)
        self.tft_drv.flush_buffer_rgb565(area.y1, area.y2, data_view)
        disp_drv.flush_ready()
    
    def round_area_cb(self, e):
        a = e.get_invalidated_area()
        a.x1 = 0
        a.x2 = 239

    def set_brightness(self, value):
        # 设置背光亮度
        if self.bl_pin == None:
            return
        if value >= 100:
            self.brightness = 100
            self.backlight.duty_u16(65535)
        elif value <= 0:
            self.brightness = 1
            self.backlight.duty_u16(655)
        else:
            self.brightness = value
            self.backlight.duty_u16(self.brightness * 655)

    def bl_off(self):
        self.backlight.duty_u16(0)
        time.sleep_ms(1)
        self.backlight.deinit()

    def bl_on(self):
        self.backlight.duty_u16(self.brightness)

    def on_sleep(self):
        # Not yet completed.
        self.bl_off()

    def on_wakeup(self):
        # Not yet completed.
        self.bl_on()



