from setuptools import setup, find_packages
from setuptools.command.build_py import build_py
import subprocess
import shutil
import os

class CustomBuildPy(build_py):
    def run(self):
        # Run make to compile binary and shared object
        repo_root = os.path.dirname(os.path.abspath(__file__))
        subprocess.run(["make"], cwd=repo_root, check=True)
        super().run()

setup(
    name="zenithpy",
    version="0.2.0",
    description="Hyper-Optimized Python Runtime & Native Assembly Engine (100% Python Compatibility)",
    author="Vashu",
    packages=["zenithpy"],
    package_dir={"": "python"},
    cmdclass={"build_py": CustomBuildPy},
    entry_points={
        "console_scripts": [
            "zenithpy = zenithpy.cli:main",
        ],
    },
)
