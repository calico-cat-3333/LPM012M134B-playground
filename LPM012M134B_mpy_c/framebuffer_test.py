import machine
from machine import Pin, PWM, Timer
from lpm012m134b_framebuf import LPM012M134B, RGB222_COLORS, color222
import framebuf
import time
import lvgl as lv

lpm = LPM012M134B(Pin(0), Pin(1), Pin(2), Pin(3), Pin(4), Pin(5), Pin(6), Pin(7), Pin(8), Pin(9), Pin(10), Pin(11), Pin(12), Pin(13), Pin(14, Pin.OUT))

lpm.fill(RGB222_COLORS.BLACK)
lpm.flush()

def blocks():
    GRID = 8          # 8x8 = 64 colors
    BOX = 20          # 每格 30 像素
    MARGIN = (240 - GRID*BOX) // 2

    for gy in range(GRID):      # green (2bit)
        for rx in range(GRID):  # red (2bit)
            # 组合 R,G,B（2bit 取值：0,85,170,255）
            r = rx * 85
            g = gy * 85
            # B 用 cell index 推出 8×8 → 共 64 种
            b = ((rx + gy) % GRID) * 85  # 任意方式生成 2bit 蓝色分量

            c = color222(r, g, b)  # 转 2-2-2 色

            # 方格坐标
            x0 = MARGIN + rx * BOX
            y0 = MARGIN + gy * BOX

            # 绘制方格（GS8 模式下，直接写像素值）
            for y in range(BOX):
                row = y0 + y
                if row >= 240:
                    break
                for x in range(BOX):
                    col = x0 + x
                    if col >= 240:
                        break
                    lpm.pixel(row, col, c)

def rgb565(r, g, b):
    nr = ((r >> 3) & 0b11111) << 11
    ng = ((g >> 2) & 0b111111) << 5
    nb = (b >> 3) & 0b11111
    return nr | ng | nb
    

blocks()
lpm.flush()

time.sleep(1)

fbuf = bytearray(80 * 80)
fb = framebuf.FrameBuffer(fbuf, 80, 80, framebuf.GS8)

fb.fill(RGB222_COLORS.RED)
lpm.blit_buffer(fb, 60, 60, 80, 80)
lpm.flush()
time.sleep(1)

fbuf1 = bytearray(40 * 40 * 2)
fb1 = framebuf.FrameBuffer(fbuf, 40, 40, framebuf.RGB565)

fb1.fill(rgb565(120,2,44))
lpm.blit_buffer_rgb565(fb1, 60, 60, 40, 40, True)
lpm.flush()
time.sleep(1)

i = 0
while True:
    lpm.fill(i)
    i = (i + 1) % 64
    lpm.flush()
    time.sleep_ms(100)