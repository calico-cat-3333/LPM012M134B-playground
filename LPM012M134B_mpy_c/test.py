import machine
from machine import Pin, PWM
import lpm012m134b
import framebuf
import time

fbuf = bytearray(240 * 240)

fb = framebuf.FrameBuffer(fbuf, 240, 240, framebuf.GS8)

p = PWM(Pin(4))
p.freq(100)
p.duty_u16(32767)

bl = Pin(14, Pin.OUT)
bl.on()

lpm = lpm012m134b.LPM012M134B(fbuf, Pin(0), Pin(1), Pin(2), Pin(3), Pin(6), Pin(7), Pin(8), Pin(9), Pin(10), Pin(11), Pin(12), Pin(13))
lpm.init()

fb.fill(lpm012m134b.BLACK)

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

            c = lpm012m134b.color222(r, g, b)  # 转 2-2-2 色

            # 方格坐标
            x0 = MARGIN + rx * BOX
            y0 = MARGIN + gy * BOX

            # 绘制方格（GS8 模式下，直接写像素值）
            for y in range(BOX):
                row = y0 + y
                if row >= 240:
                    break
                off = row * 240 + x0
                for x in range(BOX):
                    col = x0 + x
                    if col >= 240:
                        break
                    fbuf[off + x] = c

blocks()
lpm.flush()

