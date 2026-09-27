count = 0
n = 2
limit = 15000


while n <= limit:
    is_prime = 1
    d = 2
    while d * d <= n:
        if n % d == 0:
            is_prime = 0
            break
        d += 1
    if is_prime == 1:
        count += 1
    n += 1

print(count)
