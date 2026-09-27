function a_elem(i, j) {
    let ij = i + j;
    return 1.0 / ((ij * (ij + 1)) / 2 + i + 1);
}

const n = 2000;
let sum_val = 0.0;
let i = 0;
while (i < n) {
    let j = 0;
    while (j < 500) {
        sum_val += a_elem(i, j);
        j++;
    }
    i++;
}

console.log(Math.floor(sum_val));
