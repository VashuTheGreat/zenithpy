"""
ZenithPy: Hyper-Optimized Python Runtime & Native Assembly Engine
"""

__version__ = "0.2.0-alpha"

from zenithpy.accelerator import (
    asm_fib,
    asm_loop_sum,
    asm_prime_count,
    asm_vector_dot,
    asm_mandel_pixel,
    fast,
    jit,
    accelerate,
)

__all__ = [
    "asm_fib",
    "asm_loop_sum",
    "asm_prime_count",
    "asm_vector_dot",
    "asm_mandel_pixel",
    "fast",
    "jit",
    "accelerate",
]
