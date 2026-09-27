def add(a, b):
    return a + b

print(add(15, 27))

def factorial(n):
    if n <= 1:
        return 1
    return n * factorial(n - 1)

print(factorial(6))

def compute(x, y, z):
    w = x * y
    return w + z

print(compute(3, 4, 5))
