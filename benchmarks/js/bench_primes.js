let count = 0;
let n = 2;
const limit = 15000;


while (n <= limit) {
    let is_prime = 1;
    let d = 2;
    while (d * d <= n) {
        if (n % d === 0) {
            is_prime = 0;
            break;
        }
        d++;
    }
    if (is_prime === 1) {
        count++;
    }
    n++;
}

console.log(count);
