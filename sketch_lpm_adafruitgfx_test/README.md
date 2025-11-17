# 尝试接入 AdafruitGFX

代码修改自 AdafruitGFX 的示例。

在 adafruitgfx_lpm012134b.h 中将 LPM012M134B_lib 库接入 AdafruitGFX，形成 AdafruitGFX_LPM012M134B。

大概实现了一下软件旋转，调用 `setRotation` 函数调整屏幕方向后，所有绘图函数会在旋转后的位置绘图，调用前已经绘制的内容不改变位置。（还有问题没修好）

所有绘图函数颜色均为 RGB565 格式，默认使用将红蓝分量的低 3 位、绿色分量的低 4 位直接舍弃的方法转换为 RGB222 格式绘制。可以使用 `setEnableBayerDither(true)` 启用 bayer 抖动绘制，以获得更精细的颜色过度。
