尝试接入 LVGL 图形库，可能包含错误。

此版本可以在测试电路板上运行。仅在 RP2040 和 RP2350 上测试过。（必须使用 xFRP 硬件反相）

在 RP2350 上，目前开启核心 1 刷屏（`#define USE_CORE_FLUSH 1`）后，整个开发板卡死，原因正在调查，目前请使用 `#define USE_CORE_FLUSH 0` （即使是这样性能似乎依然比 RP2040 好）

支持局部刷新，使用 Bayer 抖动改善显示效果。

使用摇杆作为鼠标输入，按钮 1 和摇杆按钮为鼠标按下，按钮 2 为开关背光，按钮 3 为开关 Bayer 抖动。

尝试将刷新任务放到 Core1 上运行，实测能够少量提升帧率。

使用查找表优化 Bayer 抖动。

内容为 LVGL widgets Demo

需要在 Arduino 库管理中安装 LVGL 库，并将 lv_conf.h 复制到 Arduino 库文件夹中，还需要将 demos 和 examples 文件夹软链接到 src 文件夹中，可以通过如下命令实现：

在 Arduino/librarys/lvgl/src 文件夹中：

```
ln -s ../demos/ .
ln -s ../examples/ .
```