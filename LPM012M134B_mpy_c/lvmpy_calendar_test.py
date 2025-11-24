import lvgl as lv
import lv_utils
from lpm012m134b_lv import LPM012M134B_lv
# from adc_joystick_lv import ADC_JoyStick_lv
from machine import Pin
import time

if not lv.is_initialized(): lv.init()
if not lv_utils.event_loop.is_running(): event_loop=lv_utils.event_loop()


disp_drv = LPM012M134B_lv(Pin(0), Pin(1), Pin(2), Pin(3), Pin(4), Pin(5), Pin(6), Pin(7), Pin(8), Pin(9), Pin(10), Pin(11), Pin(12), Pin(13), Pin(14, Pin.OUT))
disp_drv.after_lvgl_init()

# indev = ADC_JoyStick_lv(Pin(27), Pin(26), Pin(22, Pin.IN), 240, 240, True, False)
# indev.after_lvgl_init()

scr = lv.obj()

lv.screen_load(scr)

cal = lv.calendar(scr)
cal.align(lv.ALIGN.CENTER,0,0)
cal.set_size(240,240)

d = lv.calendar.add_header_dropdown(cal)