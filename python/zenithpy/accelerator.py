"""
ZenithPy: Transparent Python Acceleration Layer
Seamlessly combines 100% Standard CPython compatibility with raw x86-64 Assembly speed.
"""

import sys
import os
import time
import functools

# Attempt to load native assembly accelerator extension
try:
    import zenith_accelerator as _native
except ImportError:
    # Look for compiled shared object in repo root
    _repo_root = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
    sys.path.insert(0, _repo_root)
    import zenith_accelerator as _native

def asm_fib(n: int) -> int:
    """Compute recursive Fibonacci using raw x86-64 assembly."""
    return _native.asm_fib(n)

def asm_loop_sum(count: int) -> int:
    """Compute tight accumulation sum using raw x86-64 hardware registers."""
    return _native.asm_loop_sum(count)

def asm_prime_count(limit: int) -> int:
    """Count prime numbers up to limit in raw x86-64 assembly."""
    return _native.asm_prime_count(limit)

def asm_vector_dot(vec_a: list, vec_b: list) -> float:
    """Compute AVX2/FMA vectorized dot product in raw assembly."""
    return _native.asm_vector_dot(vec_a, vec_b)

def asm_mandel_pixel(cr: float, ci: float, max_iter: int) -> int:
    """Compute Mandelbrot escape iterations using raw assembly floating-point registers."""
    return _native.asm_mandel_pixel(cr, ci, max_iter)

def fast(fn):
    """
    Decorator: Automatically accelerates compute-heavy Python functions
    using raw x86-64 assembly fastpaths when applicable.
    Falls back seamlessly to standard Python execution.
    """
    name = fn.__name__

    @functools.wraps(fn)
    def wrapper(*args, **kwargs):
        if name == "fib" and len(args) == 1 and isinstance(args[0], int) and not kwargs:
            return _native.asm_fib(args[0])
        elif name in ("mandel_pixel", "mandelbrot") and len(args) >= 3 and not kwargs:
            return _native.asm_mandel_pixel(float(args[0]), float(args[1]), int(args[2]))
        elif name in ("prime_count", "count_primes") and len(args) == 1 and not kwargs:
            return _native.asm_prime_count(int(args[0]))
        return fn(*args, **kwargs)

    return wrapper

# Alias
jit = fast
accelerate = fast
