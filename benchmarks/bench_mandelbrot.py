def mandel_pixel(cr, ci, max_iter):
    zr = 0.0
    zi = 0.0
    i = 0
    while i < max_iter:
        zr2 = zr * zr
        zi2 = zi * zi
        if zr2 + zi2 > 4.0:
            return i
        zi = 2.0 * zr * zi + ci
        zr = zr2 - zi2 + cr
        i += 1
    return max_iter

total = 0
w = 120
h = 120
y = 0
while y < h:
    ci = -1.2 + (y * 2.4) / 120.0
    x = 0
    while x < w:
        cr = -2.0 + (x * 2.5) / 120.0
        total += mandel_pixel(cr, ci, 100)
        x += 1
    y += 1

print(total)
