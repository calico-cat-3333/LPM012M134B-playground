import numpy as np
from PIL import Image

# 4x4 Bayer matrix
bayer4x4 = np.array([
    [ 0,  8,  2, 10],
    [12,  4, 14,  6],
    [ 3, 11,  1,  9],
    [15,  7, 13,  5]
])

bayer8x8 = np.array([
    [ 0, 48, 12, 60,  3, 51, 15, 63],
    [32, 16, 44, 28, 35, 19, 47, 31],
    [ 8, 56,  4, 52, 11, 59,  7, 55],
    [40, 24, 36, 20, 43, 27, 39, 23],
    [ 2, 50, 14, 62,  1, 49, 13, 61],
    [34, 18, 46, 30, 33, 17, 45, 29],
    [10, 58,  6, 54,  9, 57,  5, 53],
    [42, 26, 38, 22, 41, 25, 37, 21]
])

def bayer_dither_4gray(img):
    """
    输入: 8-bit 灰度图像（PIL Image），输出：抖动后的4级灰度图像
    """
    #img = img.convert("L")
    arr = np.array(img)
    h, w = arr.shape

    output = np.zeros_like(arr)

    for y in range(h):
        for x in range(w):
            pixel = arr[y, x]
            threshold = (bayer4x4[y % 4][x % 4] + 0.5) * 16  # 范围 0~255
            dithered = pixel + threshold - 128  # 中心化
            level = np.clip(dithered // 64, 0, 3)
            output[y, x] = level * 85  # 4 级灰度：0, 85, 170, 255

    return Image.fromarray(output, mode='L')

def ctest(i, thr):
    if i >=0 and i < 85:
        if (i > thr // 3):
            return 1
        else:
            return 0
    elif i >=85 and i < 171:
        if (i > 85 + thr // 3):
            return 2
        else:
            return 1
    else:
        if (i > 171 + thr // 3):
            return 3
        else:
            return 2

def ctest1(i, thr):
    level = i // 85
    # if i % 85 > int(thr / 3 + 0.5):
    if i % 85 > thr // 3:
        return min(level + 1, 3)
    else:
        return level

def bayer_dither8(img):
    """
    输入: 图像的R/G/B分量
    """
    #img = img.convert("L")
    arr = np.array(img)
    h, w = arr.shape

    output = np.zeros_like(arr)

    for y in range(h):
        for x in range(w):
            pixel = arr[y, x]
            threshold = (bayer8x8[y % 8][x % 8]) * 4  # 范围 0~255
            output[y, x] = ctest1(pixel, threshold) * 85  # 4 级灰度：0, 85, 170, 255

    return Image.fromarray(output, mode='L')

def bayer_dither4(img):
    """
    输入: 图像的R/G/B分量
    """
    #img = img.convert("L")
    arr = np.array(img)
    h, w = arr.shape

    output = np.zeros_like(arr)

    for y in range(h):
        for x in range(w):
            pixel = arr[y, x]
            threshold = (bayer4x4[y % 4][x % 4]) * 16  # 范围 0~255
            output[y, x] = ctest1(pixel, threshold) * 85  # 4 级灰度：0, 85, 170, 255

    return Image.fromarray(output, mode='L')

def generate_2d_color_gradient(width, height, c_tl, c_tr, c_bl, c_br):
    """
    四角分别为左上、右上、左下、右下颜色的二维线性渐变
    """
    img = Image.new("RGB", (width, height))
    for y in range(height):
        v_ratio = y / (height - 1)
        for x in range(width):
            h_ratio = x / (width - 1)
            # 水平混合
            top = [c_tl[i] * (1 - h_ratio) + c_tr[i] * h_ratio for i in range(3)]
            bottom = [c_bl[i] * (1 - h_ratio) + c_br[i] * h_ratio for i in range(3)]
            # 垂直混合
            color = tuple(int(top[i] * (1 - v_ratio) + bottom[i] * v_ratio) for i in range(3))
            img.putpixel((x, y), color)
    return img

def generate_gray_gradient(width, height):
    img = Image.new('L', (width, height))  # 'L' 模式表示灰度图
    for x in range(width):
        gray = int(255 * x / (width - 1))
        for y in range(height):
            img.putpixel((x, y), gray)
    return img

# img = generate_2d_color_gradient(
#     512, 512,
#     (255, 0, 0), (0, 255, 0),  # 上：红 -> 绿
#     (0, 0, 255), (255, 255, 0) # 下：蓝 -> 黄
# )
#
# img = Image.open("timg.png")
# img.show()
#
# r, g, b = img.split()
#
# orc = bayer_dither4(r)
# ogc = bayer_dither4(g)
# obc = bayer_dither4(b)
#
# o = Image.merge('RGB', (orc, ogc, obc))
#
# # img = generate_gray_gradient(512, 512)
# # img.show()
#
# # o = bayer_dither(img)
#
# o.show()
#
# orc1 = bayer_dither8(r)
# ogc1 = bayer_dither8(g)
# obc1 = bayer_dither8(b)
#
# o1 = Image.merge('RGB', (orc1, ogc1, obc1))
# o1.show()

print('const uint8_t bayer_lut[64][4][4] = {')
for i in range(0, 1<<6):
    print('{', end='')
    for y in range(0, 4):
        print('{', end='')
        for x in range(0, 4):
            thr = (bayer4x4[y % 4][x % 4]) * 16
            o = ctest1(i<<2, thr)
            if x == 3:
                print(o, end='')
            else:
                print(o, end=', ')
        if y == 3:
            print('}', end='')
        else:
            print('}', end=', ')
    print('},')
print('};')

print('const uint32_t compressed_bayer_lut[64] = {')
for i in range(0, 1<<6):
    o=0
    for y in range(0, 4):
        for x in range(0, 4):
            thr = (bayer4x4[y % 4][x % 4]) * 16
            ox = ctest1(i<<2, thr)
            o = o | (ox << ((x+y*4)*2))
    if (i % 8 != 7):
        print(o, end=', ')
    else:
        print(o, end=',\n')
print('};')
