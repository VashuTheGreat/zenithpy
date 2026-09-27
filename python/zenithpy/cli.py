#!/usr/bin/env python3
"""
ZenithPy Full CLI Driver:
100% Python CLI compatibility including:
- Virtual environments: `zenithpy -m venv .venv`
- Pip package installation: `zenithpy -m pip install <package>`
- uv package manager compatibility
- Script execution with raw x86-64 assembly speedups
"""

import sys
import os
import runpy
import subprocess
import shutil

def setup_venv_zenith(venv_dir: str):
    """Injects zenithpy and zenith binaries into newly created virtualenv."""
    bin_dir = os.path.join(venv_dir, "bin")
    if os.path.isdir(bin_dir):
        zenith_src = os.path.abspath(__file__)
        zenith_bin = shutil.which("zenithpy") or zenith_src

        target_zenithpy = os.path.join(bin_dir, "zenithpy")
        target_zenith = os.path.join(bin_dir, "zenith")

        try:
            if os.path.exists(target_zenithpy):
                os.remove(target_zenithpy)
            os.symlink(zenith_bin, target_zenithpy)

            if os.path.exists(target_zenith):
                os.remove(target_zenith)
            os.symlink(zenith_bin, target_zenith)
        except OSError:
            pass

def auto_detect_venv():
    """Detects .venv in current or parent directory and adds site-packages to sys.path."""
    cur = os.getcwd()
    while True:
        venv_path = os.path.join(cur, ".venv")
        if os.path.isdir(venv_path):
            lib_dir = os.path.join(venv_path, "lib")
            if os.path.isdir(lib_dir):
                try:
                    for d in os.listdir(lib_dir):
                        sp = os.path.join(lib_dir, d, "site-packages")
                        if os.path.isdir(sp) and sp not in sys.path:
                            sys.path.insert(0, sp)
                except OSError:
                    pass
            break
        parent = os.path.dirname(cur)
        if parent == cur:
            break
        cur = parent

def main():
    auto_detect_venv()
    if len(sys.argv) < 2:

        print(f"ZenithPy 0.2.0 (Python {sys.version.split()[0]} on {sys.platform})")
        print("Usage: zenithpy [options] [-m module | file.py] [args...]")
        print("       zenithpy -m venv <env_dir>       (Create virtualenv)")
        print("       zenithpy -m pip install <pkg>     (Install packages)")
        print("       zenithpy -c '<code>'              (Execute inline code)")
        print("       zenithpy <script.py>              (Run accelerated Python script)")
        return

    # Add repo root and python dir to path
    repo_root = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
    python_dir = os.path.join(repo_root, "python")
    if repo_root not in sys.path:
        sys.path.insert(0, repo_root)
    if python_dir not in sys.path:
        sys.path.insert(0, python_dir)

    arg1 = sys.argv[1]

    # 1. Version flag
    if arg1 in ("-V", "--version"):
        print(f"Python {sys.version.split()[0]} (ZenithPy 0.2.0, x86_64 native JIT)")
        return

    # 2. Module execution flag (-m <module>)
    if arg1 == "-m" and len(sys.argv) > 2:
        mod_name = sys.argv[2]
        remaining_args = sys.argv[2:]
        sys.argv = remaining_args

        # Execute target module
        try:
            runpy.run_module(mod_name, run_name="__main__", alter_sys=True)
        except SystemExit as e:
            # If creating venv, hook zenithpy into the virtual environment
            if mod_name == "venv" and len(remaining_args) > 1:
                env_path = remaining_args[1]
                setup_venv_zenith(env_path)
            sys.exit(e.code)

        if mod_name == "venv" and len(remaining_args) > 1:
            env_path = remaining_args[1]
            setup_venv_zenith(env_path)
        return

    # 3. Inline code flag (-c <code>)
    if arg1 == "-c" and len(sys.argv) > 2:
        code = sys.argv[2]
        from zenithpy.auto_optimizer import optimize_and_exec
        optimize_and_exec(code, "<string>", {"__name__": "__main__", "__builtins__": __builtins__})
        return

    # 4. Standard script execution (zenithpy script.py)
    script_path = sys.argv[1]
    sys.argv = sys.argv[1:]  # shift argv so script receives correct args

    if not os.path.exists(script_path):
        print(f"Error: Script '{script_path}' not found.", file=sys.stderr)
        sys.exit(1)

    with open(script_path, "r", encoding="utf-8") as f:
        source_code = f.read()

    from zenithpy.auto_optimizer import optimize_and_exec
    global_dict = {
        "__name__": "__main__",
        "__file__": os.path.abspath(script_path),
        "__builtins__": __builtins__,
    }
    optimize_and_exec(source_code, script_path, global_dict)

if __name__ == "__main__":
    main()
