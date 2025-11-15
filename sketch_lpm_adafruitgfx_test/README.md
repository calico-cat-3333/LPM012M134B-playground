# 尝试接入 AdafruitGFX

代码修改自 AdafruitGFX 的示例。

adafruitgfx_lpm012134b.h 中包含接入了 AdafruitGFX 的 LPM012M134B 驱动，他依赖 LPM012M134B_lib 库。

大概实现了一下软件旋转，调用 `setRotation` 函数调整屏幕方向后，再绘制的像素点会绘制到旋转后的位置上，调用前绘制的像素点不改变位置。

所有绘图函数颜色均为 RGB565 格式，内部使用将红蓝分量的低 3 位，绿色分量的低 4 位直接舍弃的方法转换为 RGB222 格式绘制。

// todo: 支持使用 bayer 抖动绘制 RGB565 颜色的图形，并可以在两种模式之间切换。
