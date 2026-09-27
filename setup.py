from setuptools import setup, Extension
import os

extra_compile_args = ["-O3", "-march=native", "-mavx2", "-mfma"]

zenith_module = Extension(
    "zenith_accelerator",
    sources=["src/zenith_extension.c", "src/asm/fastpath_x86_64.s"],
    extra_compile_args=extra_compile_args,
)

setup(
    name="zenithpy",
    version="0.1.0",
    description="Hyper-Optimized Python Runtime & Native Assembly Engine",
    ext_modules=[zenith_module],
    packages=["zenithpy"],
    package_dir={"": "python"},
)
