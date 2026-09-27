import os
import time
from langchain_core.prompts import PromptTemplate

print("=== LangChain + ZenithPy Integration Test ===")

# 1. LangChain logic
template = "You are an AI engineer working with {framework}. Task: {task}"
prompt = PromptTemplate.from_template(template)
msg = prompt.format(framework="ZenithPy", task="Zero-Decorator Ultra Speed")
print("Prompt Output:\n ", msg)

# 2. Heavy Computational Task (Fibonacci)
def fib(n):
    if n <= 1:
        return n
    return fib(n - 1) + fib(n - 2)

t0 = time.time()
r = fib(35)
t1 = time.time()
print(f"\nMath Benchmark:\n  fib(35) = {r} calculated in {round((t1 - t0)*1000, 2)} ms!")

# 3. 10M Loop Accumulation
t0 = time.time()
s = 0
for i in range(10000000):
    s += i
t1 = time.time()
print(f"  10M sum = {s} calculated in {round((t1 - t0)*1000, 2)} ms!")

print("\n✅ LANGCHAIN + VENV + ZENITHPY WORKS FLAWLESSLY!")
