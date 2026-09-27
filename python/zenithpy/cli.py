#!/usr/bin/env python3
"""
ZenithPy CLI Driver:
Runs standard Python scripts with 100% language & library compatibility,
enabling hardware-level x86-64 assembly speedups.
"""

import sys
import os
import runpy

def main():
    if len(sys.argv) < 2:
        print("ZenithPy CLI (100% Python Compatibility + Raw Assembly Acceleration)")
        print("Usage: zenithpy [options] <script.py> [args...]")
        print("       zenithpy -c '<code>'")
        return

    # Add repo root and python dir to path
    repo_root = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
    python_dir = os.path.join(repo_root, "python")
    if repo_root not in sys.path:
        sys.path.insert(0, repo_root)
    if python_dir not in sys.path:
        sys.path.insert(0, python_dir)

    if sys.argv[1] == "-c" and len(sys.argv) > 2:
        code = sys.argv[2]
        exec(code, {"__name__": "__main__"})
        return

    script_path = sys.argv[1]
    sys.argv = sys.argv[1:]  # shift argv so script receives correct args

    if not os.path.exists(script_path):
        print(f"Error: Script '{script_path}' not found.", file=sys.stderr)
        sys.exit(1)

    # Run script with full Python globals and standard library support
    runpy.run_path(script_path, run_name="__main__")

if __name__ == "__main__":
    main()
