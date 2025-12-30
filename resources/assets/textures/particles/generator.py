from PIL import Image
import math

# 创建一个空白图像，模式为RGBA，白色背景
img = Image.new('RGBA', (512, 512), (255, 255, 255, 0))

# 获取图像的中间点
center_x, center_y = img.size[0] // 2, img.size[1] // 2

# 定义最大距离（对角线的一半），用于归一化alpha值
max_distance = center_x

for x in range(img.size[0]):
    for y in range(img.size[1]):
        # 计算当前点到中心的距离
        distance_from_center = math.sqrt((x - center_x) ** 2 + (y - center_y) ** 2)
        
        # 根据距离计算alpha值，越远离中心alpha值越低
        alpha = 255
        if distance_from_center > max_distance:
            alpha = 0
        else:
            alpha = math.exp(-distance_from_center**2 / (2 * (128)**2))
            alpha = (alpha * 256)
        
        
        img.putpixel((x, y), (255,255,255,int(alpha)));

# 保存图像
img.save('output.png', 'PNG')