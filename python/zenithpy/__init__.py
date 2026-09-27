"""
ZenithPy: Ultra-High-Performance Python Runtime & Native JIT Engine
"""

__version__ = "0.1.0-alpha"

import subprocess
import sys
import os

BIN_PATH = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", "bin", "zenithpy"))

def run_script(script_path, *args):
    if not os.path.exists(BIN_PATH):
        raise RuntimeError("ZenithPy binary not found. Run 'make' first.")
    cmd = [BIN_PATH, script_path] + list(args)
    return subprocess.run(cmd)

def main():
    if not os.path.exists(BIN_PATH):
        print(f"ZenithPy binary not found at {BIN_PATH}. Building now...", file=sys.stderr)
        repo_root = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
        subprocess.run(["make"], cwd=repo_root, check=True)
    cmd = [BIN_PATH] + sys.argv[1:]
    res = subprocess.run(cmd)
    sys.exit(res.returncode)

if __name__ == "__main__":
    main()
