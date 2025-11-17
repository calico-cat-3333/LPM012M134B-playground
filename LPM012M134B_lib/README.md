# LPM012M134B_lib

驱动库。

提供 API:

### `LPM012M134B(int vst, int vck, int enb, int xrst, int frp, int xfrp, int hst, int hck, int r1, int r2, int g1, int g2, int b1, int b2)`

构造函数。参数为引脚编号。

### `void init()`

初始化设备。

### `void flush_buffer_rgb565(int y1, int y2, uint16_t *buf)`

将一个 RGB565 格式的缓冲区刷新到屏幕，缓冲区的宽度必须为 240, y1 为目标区域开始行, y2 为目标区域结束行。

这个函数主要是给 lvgl flush callback 用的，这种情况下 y1 = area.y1, y2 = area.y2, buf = px_map, 并需要使用如下代码确保缓冲区宽度为 240：

```
void rounder_event_cb(lv_event_t * e)
{
  lv_area_t * a = lv_event_get_invalidated_area(e);

  a->x1 = 0;
  a->x2 = TFT_HOR_RES - 1;
}

// in setup():
lv_display_add_event_cb(disp, rounder_event_cb, LV_EVENT_INVALIDATE_AREA, NULL);
```

### `uint16_t bayer_dither_point(int x, int y, uint16_t rgb565)`

计算对位置为 x, y 的点使用 bayer 抖动后的颜色，注意，返回值为 RGB565 格式。

### `void bayer_dither_buffer(int x, int y, int w, int h, uint16_t * buf)`

对将绘制到 x, y, 高度宽度为 w, h 的 RGB565 格式的缓冲区 buf 做 bayer 抖动处理，此函数将直接修改 buf 缓冲区。

### `int8_t rgb565_to_rgb222(uint16_t rgb565)`

使用直接舍弃低位的方法将 RGB565 颜色转换为 RGB222 颜色。

### `LPM012M134B_USE_FRAMEBUFFER`

在导入该库头文件之前设置此定义可以配置是否启用下面的 framebuffer 相关的函数。默认在未导入 lvgl 时启用，导入 lvgl 时禁用以节省内存。

### `void flush(int rstart=0, int height=240)`

刷新屏幕，支持局部刷新，rstart 为刷新开始行，height 为刷新区域高度。

### `void fill(int8_t rgb222)`

将缓冲区填充为 rgb222 颜色。

### `void drawPixel(int x, int y, int8_t rgb222)`

在 x, y 位置绘制 rgb222 颜色的点。

### `void drawFastHLine(int x, int y, int w, int8_t rgb222)`

以 x, y 为起点，绘制 rgb222 颜色，宽 w 的横线。

### `void drawFastVLine(int x, int y, int h, int8_t rgb222)`

以 x, y 为起点，绘制 rgb222 颜色，高 h 的竖线。

### `void drawLine(int x0, int y0, int x1, int y1, int8_t rgb222)`

绘制一条从 x0, y0 到 x1, y1 的颜色为 rgb222 的直线。

### `void drawRect(int x, int y, int w, int h, int8_t rgb222)`

以 x, y 为起点，绘制 rgb222 颜色，宽 w 高 h 的矩形。

### `void drawEllipse(int xc, int yc, int a, int b, int8_t rgb222)`

以 xc, yc 为中点，绘制 rgb222 颜色，半长轴长 a 半短轴长 b 的椭圆。

### `void drawPixelRGB565(int x, int y, uint16_t rgb565)`

在 x, y 位置绘制 rgb565 颜色的点，使用 bayer 抖动。

### `void drawFastHLineRGB565(int x, int y, int w, uint16_t rgb565)`

以 x, y 为起点，绘制 rgb565 颜色，宽 w 的横线，使用 bayer 抖动。

### `void drawFastVLineRGB565(int x, int y, int h, uint16_t rgb565)`

以 x, y 为起点，绘制 rgb565 颜色，高 h 的竖线，使用 bayer 抖动。

### `void drawLineRGB565(int x0, int y0, int x1, int y1, uint16_t rgb565)`

绘制一条从 x0, y0 到 x1, y1 的颜色为 rgb565 的直线，使用 bayer 抖动。

### `void drawRectRGB565(int x, int y, int w, int h, uint16_t rgb565)`

以 x, y 为起点，绘制 rgb565 颜色，宽 w 高 h 的矩形，使用 bayer 抖动。

### `void drawEllipseRGB565(int xc, int yc, int a, int b, uint16_t rgb565)`

以 xc, yc 为中点，绘制 rgb565 颜色，半长轴长 a 半短轴长 b 的椭圆，使用 bayer 抖动。

### `void fillRGB565(uint16_t rgb565)`

将缓冲区填充为 rgb565 颜色，使用 bayer 抖动。

