# MicroPython C 驱动程序

适用于 MicroPython 的驱动程序，使用 C 编写以提高效率。

仍需进一步完善和测试。

目前需要单独为 frp 引脚启用 PWM.

## 编译 MicroPython

### MicroPython 原版

在同一个文件夹中依次克隆 micropython 和 LPM012M134B_playground

按照目标端口的需要进行，编译时添加参数 `USER_C_MODULES=../../../LPM012M134B_playground/LPM012M134B_mpy_c/micropython.cmake`

### lv_micropython

lv_micropython 本身需要使用该参数添加 lvgl 模块，所以则需要准备一个 bind.cmake 文件将 lvgl 和屏幕驱动都添加进去，文件内容为：

```
include(${CMAKE_CURRENT_LIST_DIR}/lv_micropython/user_modules/lv_binding_micropython/bindings.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/LPM012M134B_playground/LPM012M134B_mpy_c/micropython.cmake)
```

然后按照目标端口的需要进行，编译时添加参数 `USER_C_MODULES=../../../bind.cmake`

## 使用

注意：由于屏幕还需要为 frp, bl 等引脚进行单独配置，因此不推荐直接使用此模块，而是推荐使用 lpm012m134b_framebuf 和 lpm012m134b_lv 两个扩展后的模块：

[lpm012m134b_framebuf.py](lpm012m134b_framebuf.py) 将 framebuf 和屏幕驱动结合。可以参考 [framebuffer_test.py](framebuffer_test.py) 使用。

[lpm012m134b_lv.py](lpm012m134b_lv.py) 接入 lv_micropython 显示驱动。可以参考 [lvmpy_calendar_test.py](framebuffer_test.py) 使用。

如果直接使用，需要为 frp 启用 60 Hz 或 100 Hz 50% 占空比的 PWM 输出，bl 需要配置为高电平输出（启用背光）或低电平输出（关闭背光）

## API 说明：

<details>
<summary>lpm012m134b_framebuf 模块</summary>

### RGB222_COLORS 颜色常量

预定义的 RGB222 RGB 颜色常量：

- WHITE = 0b111111 (白色)
- BLACK = 0b000000 (黑色)
- RED = 0b110000 (红色)
- GREEN = 0b001100 (绿色)
- BLUE = 0b000011 (蓝色)
- YELLOW = 0b111100 (黄色)
- CYAN = 0b001111 (青色)
- MAGENTA = 0b110011 (洋红色)

### LPM012M134B 类

#### 构造函数 `LPM012M134B(vst, vck, enb, xrst, frp, xfrp, hst, hck, r1, r2, g1, g2, b1, b2, bl)`

初始化显示器驱动实例。

参数：

- vst, vck, enb, xrst: 垂直控制引脚
- frp: FRP 引脚（用于 PWM 控制）
- xfrp: XFRP 引脚（当前未使用）
- hst, hck: 水平控制引脚
- r1, r2, g1, g2, b1, b2: RGB 数据引脚
- bl: 背光控制引脚

特性：

- 自动创建 240×240 的帧缓冲区
- 使用 GS8 帧缓冲区格式（每像素 8 位，实际使用低 6 位作为 RGB222）
- 自动初始化 FRP PWM（频率 100Hz，占空比 50%
- 启用背光
- 初始化底层显示设备

#### `flush(rstart=0, height=240)`

刷新屏幕显示内容。

参数：

- rstart (int): 起始行索引，默认为 0
- height (int): 刷新区域高度，默认为 240（整个屏幕）

将帧缓冲区的内容刷新到物理显示器。支持部分区域刷新以提高性能。

#### `blit_buffer(buffer, x, y, w, h)`

将 RGB222 颜色格式的缓冲区数据复制到帧缓冲区。

参数：

- buffer (bytearray): 源数据缓冲区（RGB222 格式）
- x (int): 目标区域 X 坐标
- y (int): 目标区域 Y 坐标
- w (int): 目标区域宽度
- h (int): 目标区域高度

将外部缓冲区数据复制到帧缓冲区的指定位置。

#### `blit_buffer_rgb565(buffer, x, y, w, h, use_bayer=False)`

将 RGB565 格式的缓冲区数据复制到帧缓冲区。

参数：

- buffer (bytearray): 源数据缓冲区（RGB565 格式）
- x (int): 目标区域 X 坐标
- y (int): 目标区域 Y 坐标
- w (int): 目标区域宽度
- h (int): 目标区域高度
- use_bayer (bool): 是否应用拜耳抖动，默认为 False

将 RGB565 格式数据转换为 RGB222 格式并复制到帧缓冲区，可选应用拜耳抖动改善颜色表现。

#### 继承的 FrameBuffer 方法

由于继承自 framebuf.FrameBuffer，该类支持所有标准帧缓冲区操作，具体请参考 [framebuf 文档](https://docs.micropython.org/en/latest/library/framebuf.html)

### 导入的函数

从底层模块导入的实用函数：

- `bayer_dither_point(x, y, rgb565)`: 对指定坐标点的 RGB565 颜色应用拜耳抖动算法。
- `bayer_dither_buffer(x, y, w, h, buffer)`: 对将写入特定位置的缓冲区应用拜耳抖动算法。
- `color222(r, g, b)`: 将 8 位 RGB 转换为 RGB222 格式
- `color565to222(rgb565)`: 将 RGB565 转换为 RGB222 格式

具体请参考下文 lpm012m134b C 模块章节。

</details>

<details>
<summary>lpm012m134b_lv 模块</summary>

### LPM012M134B_lv 类

#### 构造函数 `LPM012M134B_lv(vst, vck, enb, xrst, frp, xfrp, hst, hck, r1, r2, g1, g2, b1, b2, bl, doublebuffer=False, factor=4)`

初始化 LVGL 集成的显示器驱动实例。

参数：

- vst, vck, enb, xrst: 垂直控制引脚
- frp: FRP 引脚（PWM 控制）
- xfrp: XFRP 引脚（当前未使用）
- hst, hck: 水平控制引脚
- r1, r2, g1, g2, b1, b2: RGB 数据引脚
- bl: 背光控制引脚（PWM，可为 None）
- doublebuffer (bool): 是否启用双缓冲，默认为 False
- factor (int): 缓冲区高度分割因子，默认为 4

特性：

- 显示器分辨率：240 × 240 像素
- 自动初始化 FRP PWM（频率 100Hz，占空比 50%）
- 支持背光 PWM 控制（频率 5kHz）
- 初始化底层显示设备
- 默认背光亮度：100%

#### `after_lvgl_init()`

完成 LVGL 显示驱动的初始化配置。必须在 LVGL 初始化后调用。

配置内容：

- 设置颜色格式为 RGB565
- 创建绘制缓冲区（单缓冲或双缓冲）
- 设置渲染模式为部分渲染
- 注册刷新回调函数
- 注册区域修整事件回调

注意： 此方法必须在 lv.init() 之后调用。

#### `set_brightness(value)`

设置背光亮度。

参数：

- value (int): 亮度值 (0-100)

特性：

- 值 ≥ 100：最大亮度（100%）
- 值 ≤ 0：最低亮度（1%）
- 中间值：线性映射到 PWM 占空比
- 如果背光引脚为 None，则忽略调用

#### `bl_off()`

关闭背光。

操作：

- 设置 PWM 占空比为 0%
- 延迟 1ms 确保稳定
- 反初始化背光 PWM

#### `bl_on()`

开启背光。

操作：

- 重新启用背光 PWM
- 恢复到最后设置的亮度值

</details>

<details>
<summary>lpm012m134b C 模块</summary>

### 实用函数

#### `color222(r, g, b)`

将 8 位 RGB 颜色值转换为 RGB222 颜色格式。

参数：

- r (int): 红色分量 (0-255)
- g (int): 绿色分量 (0-255)
- b (int): 蓝色分量 (0-255)

返回：

- int: RGB222 格式的颜色值 (6位)

#### `color565to222(rgb565)`

将 16 位 RGB565 颜色值转换为 RGB222 颜色格式。

参数：

- rgb565 (int): RGB565 格式的颜色值

返回：

- int: RGB222 格式的颜色值

#### `bayer_dither_point(x, y, rgb565)`

对指定坐标点的 RGB565 颜色应用拜耳抖动算法。

参数：

- x (int): X 坐标
- y (int): Y 坐标
- rgb565 (int): RGB565 格式的颜色值

返回：

- int: 抖动处理后的 RGB565 颜色值

#### `bayer_dither_buffer(x, y, w, h, buffer)`

对将写入特定位置的缓冲区应用拜耳抖动算法。

参数：

- x (int): 区域起始 X 坐标
- y (int): 区域起始 Y 坐标
- w (int): 区域宽度
- h (int): 区域高度
- buffer (bytearray): RGB565 颜色缓冲区（可读写）

### LPM012M134B 类

#### 构造函数 `LPM012M134B(fbuf, vst, vck, enb, xrst, hst, hck, r1, r2, g1, g2, b1, b2)`

参数：

- fbuf (bytearray): 帧缓冲区，传入 None 禁用帧缓冲区
- vst, vck, enb, xrst, hst, hck: 控制引脚
- r1, r2, g1, g2, b1, b2: 数据引脚

#### `init()`

初始化显示器引脚配置。将所有控制引脚和数据引脚设置为输出模式并置为低电平。

#### `flush([start[, height]])`

刷新屏幕显示内容。支持部分区域刷新。

参数：

- start (int, 可选): 起始行索引，默认为 0
- height (int, 可选): 刷新区域高度，默认为屏幕高度

异常：

- 如果帧缓冲区被禁用，抛出 ValueError

#### `flush_buffer_rgb565(y1, y2, buffer)`

直接将 RGB565 缓冲区内容刷新到屏幕指定区域。适用于 LVGL 等图形库的刷新回调函数。

参数：

- y1 (int): 区域起始 Y 坐标（LVGL 的 area->y1）
- y2 (int): 区域结束 Y 坐标（LVGL 的 area->y2）
- buffer (bytearray): RGB565 格式的像素数据缓冲区

注意：

刷新区域必须为整行，因此需要以下代码来限制 LVGL 刷新区域宽度：

```
def round_area_cb(e):
    a = e.get_invalidated_area()
    a.x1 = 0
    a.x2 = 239

disp_drv.add_event_cb(round_area_cb, lv.EVENT.INVALIDATE_AREA, None)
```

#### `blit_buffer(buffer, x, y, h, w)`

将 RGB222 颜色格式的缓冲区数据复制到帧缓冲区。

参数：

- buffer (bytearray): RGB222 格式的源数据缓冲区
- x (int): 目标区域 X 坐标
- y (int): 目标区域 Y 坐标
- h (int): 目标区域高度
- w (int): 目标区域宽度

异常：

- 如果帧缓冲区被禁用，抛出 ValueError

#### `blit_buffer_rgb565(buffer, x, y, h, w[, use_bayer=False])`

将 RGB565 缓冲区数据复制到帧缓冲区，可选的拜耳抖动处理。

参数：

- buffer (bytearray): RGB565 格式的源数据缓冲区
- x (int): 目标区域 X 坐标
- y (int): 目标区域 Y 坐标
- h (int): 目标区域高度
- w (int): 目标区域宽度
- use_bayer (bool, 可选): 是否应用拜耳抖动，默认为 False

异常：

- 如果帧缓冲区被禁用，抛出 ValueError

### 颜色常量

模块提供了以下预定义颜色常量（RGB222 格式）：

- BLACK: 黑色
- BLUE: 蓝色
- RED: 红色
- GREEN: 绿色
- CYAN: 青色
- MAGENTA: 洋红色
- YELLOW: 黄色
- WHITE: 白色

</details>
