function mandel_pixel(cr, ci, max_iter) {
    let zr = 0.0;
    let zi = 0.0;
    let i = 0;
    while (i < max_iter) {
        let zr2 = zr * zr;
        let zi2 = zi * zi;
        if (zr2 + zi2 > 4.0) {
            return i;
        }
        zi = 2.0 * zr * zi + ci;
        zr = zr2 - zi2 + cr;
        i++;
    }
    return max_iter;
}

let total = 0;
const w = 120;
const h = 120;
let y = 0;
while (y < h) {
    let ci = -1.2 + (y * 2.4) / 120.0;
    let x = 0;
    while (x < w) {
        let cr = -2.0 + (x * 2.5) / 120.0;
        total += mandel_pixel(cr, ci, 100);
        x++;
    }
    y++;
}

console.log(total);
