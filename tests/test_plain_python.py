import time
import os
import math

print("=== 1. Standard Python Imports & Builtins ===")
print("  OS Name:", os.name)
print("  Math Sqrt(256):", math.sqrt(256))

print("\n=== 2. Plain Fibonacci (Zero Decorators, Zero Zenith Imports) ===")
def fib(n):
    if n <= 1:
        return n
    return fib(n - 1) + fib(n - 2)

t0 = time.time()
ans = fib(35)
t1 = time.time()
print(f"  Result: fib(35) = {ans}")
print(f"  Execution Time: {round((t1 - t0) * 1000, 2)} ms ({round(t1 - t0, 4)} s)")

print("\n=== 3. Plain 10 Million Loop Sum (Zero Decorators) ===")
t0 = time.time()
s = 0
for i in range(10000000):
    s += i
t1 = time.time()
print(f"  Result: 10M sum = {s}")
print(f"  Execution Time: {round((t1 - t0) * 1000, 2)} ms ({round(t1 - t0, 4)} s)")

print("\n=== 4. Plain Prime Counting (Zero Decorators) ===")
def count_primes(limit):
    count = 0
    n = 2
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
    return count

t0 = time.time()
p_count = count_primes(15000)
t1 = time.time()
print(f"  Result: Primes <= 15000 = {p_count}")
print(f"  Execution Time: {round((t1 - t0) * 1000, 2)} ms ({round(t1 - t0, 4)} s)")

print("\n=== 5. Arbitrary Arithmetic Loop (total += (i * i) % 7) ===")
t0 = time.time()
total_math = 0
for i in range(5000000):
    total_math += (i * i) % 7
t1 = time.time()
print(f"  Result: 5M Math Loop = {total_math}")
print(f"  Execution Time: {round((t1 - t0) * 1000, 2)} ms ({round(t1 - t0, 4)} s)")

