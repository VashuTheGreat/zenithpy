s = 0
for i in range(10):
    s += i
print(s)

s = 0
i = 0
while i < 10:
    s += i
    i += 1
print(s)

count = 0
for i in range(20):
    if i == 5:
        continue
    if i == 12:
        break
    count += 1
print(count)
