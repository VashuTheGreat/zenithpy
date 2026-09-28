let total = 0;
for (let i = 0; i < 5000000; i++) {
    total += (i * i) % 7;
}
console.log(total);
