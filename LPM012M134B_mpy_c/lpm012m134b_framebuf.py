from lpm012m134b import LPM012M134B as _LPM012M134B
from lpm012m134b import bayer_dither_point, bayer_dither_buffer, color222, color565to222
from machine import Pin, PWM
import framebuf

class RGB222_COLORS:
    WHITE   = 0b111111
    BLACK   = 0b000000
    RED     = 0b110000
    GREEN   = 0b001100
    BLUE    = 0b000011
    YELLOW  = 0b111100
    CYAN    = 0b001111
    MAGENTA = 0b110011

class LPM012M134B(framebuf.FrameBuffer):
    def __init__(self, vst, vck, enb, xrst, frp, xfrp, hst, hck, r1, r2, g1, g2, b1, b2, bl):
        self.width = 240
        self.height = 240
        self.fbuf = bytearray(self.width * self.height)
        super().__init__(self.fbuf, self.width, self.height, framebuf.GS8)

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

        self.bl_pin.on()

        self.dispdev = _LPM012M134B(self.fbuf, vst, vck, enb, xrst, hst, hck, r1, r2, g1, g2, b1, b2)
        self.dispdev.init()
    
    def flush(self, rstart=0, height=240):
        self.dispdev.flush(rstart, height)
    
    def blit_buffer(self, buffer, x, y, w, h):
        self.dispdev.blit_buffer(buffer, x, y, w, h)
    
    def blit_buffer_rgb565(self, buffer, x, y, w, h, use_bayer=False):
        self.dispdev.blit_buffer_rgb565(buffer, x, y, w, h, use_bayer)

