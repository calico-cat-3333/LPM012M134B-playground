# MicroPython C 驱动程序

适用于 MicroPython 的驱动程序，使用 C 编写以提高效率。

尚未完成，等待进一步完善和测试。

目前需要单独为将 frp 引脚启用 PWM.

需重新编译 MicroPython.

lpm012m134b_framebuf.py 是 python 部分的胶水代码，将 framebuf 和屏幕驱动结合。可以参考 framebuffer_test.py 使用。

lpm012m134b_lv.py 是 python 部分的胶水代码，接入 lv_micropython 显示驱动。可以参考 lvmpy_calendar_test.py 使用。