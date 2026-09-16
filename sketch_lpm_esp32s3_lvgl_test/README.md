尝试接入 LVGL 图形库，可能包含错误。

支持局部刷新，使用 Bayer 抖动改善显示效果。

使用查找表优化 Bayer 抖动。

内容为 LVGL Benchmark Demo

需要在 Arduino 库管理中安装 LVGL 库，并将 lv_conf.h 复制到 Arduino 库文件夹中，还需要将 demos 和 examples 文件夹软链接到 src 文件夹中，可以通过如下命令实现：

在 Arduino/librarys/lvgl/src 文件夹中：

```
ln -s ../demos/ .
ln -s ../examples/ .
```
