def a_elem(i, j):
    ij = i + j
    return 1.0 / ((ij * (ij + 1)) / 2 + i + 1)

n = 2000
sum_val = 0.0
i = 0
while i < n:
    j = 0
    while j < 500:
        sum_val += a_elem(i, j)
        j += 1
    i += 1

print(int(sum_val))
