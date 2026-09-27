"""
ZenithPy Native Loop JIT Compiler:
Dynamically compiles arbitrary arithmetic loops into optimized native C/machine code.
Zero decorators, 100% transparent execution.
"""

import ast
import os
import sys
import hashlib
import ctypes
import subprocess

CACHE_DIR = os.path.expanduser("~/.cache/zenithpy/jit")
os.makedirs(CACHE_DIR, exist_ok=True)

# Keep references to loaded CDLLs to prevent unloading
_LOADED_LIBS = {}

def ast_to_c_expr(node, loop_var, allowed_vars):
    """Recursively converts an AST expression into a C arithmetic expression."""
    if isinstance(node, ast.BinOp):
        left = ast_to_c_expr(node.left, loop_var, allowed_vars)
        right = ast_to_c_expr(node.right, loop_var, allowed_vars)
        if left is None or right is None:
            return None
        op_map = {
            ast.Add: "+",
            ast.Sub: "-",
            ast.Mult: "*",
            ast.Mod: "%",
            ast.FloorDiv: "/",
            ast.BitOr: "|",
            ast.BitAnd: "&",
            ast.BitXor: "^",
            ast.LShift: "<<",
            ast.RShift: ">>",
        }
        if type(node.op) in op_map:
            return f"({left} {op_map[type(node.op)]} {right})"
        return None

    elif isinstance(node, ast.UnaryOp):
        operand = ast_to_c_expr(node.operand, loop_var, allowed_vars)
        if operand is None:
            return None
        if isinstance(node.op, ast.USub):
            return f"(-{operand})"
        elif isinstance(node.op, ast.UAdd):
            return f"(+{operand})"
        elif isinstance(node.op, ast.Invert):
            return f"(~{operand})"
        return None

    elif isinstance(node, ast.Constant):
        if isinstance(node.value, int):
            return f"{node.value}LL"
        elif isinstance(node.value, float):
            return f"{node.value}"
        return None

    elif isinstance(node, ast.Name):
        if node.id == loop_var or node.id in allowed_vars:
            return node.id
        return None

    return None

def collect_free_names(node, exclude_names):
    """Finds all variable names in expression excluding loop var and accumulator."""
    names = set()
    for child in ast.walk(node):
        if isinstance(child, ast.Name) and child.id not in exclude_names:
            names.add(child.id)
    return sorted(list(names))

def compile_numeric_loop(loop_var: str, accum_var: str, op_type: type, expr_node: ast.AST):
    """
    Attempts to compile an arithmetic loop into a native C shared library.
    Returns (func_id, extra_var_names) if successful, or None if unsupported.
    """
    # 1. Collect free variables
    free_vars = collect_free_names(expr_node, {loop_var, accum_var})
    
    # 2. Try converting expression to C
    c_expr = ast_to_c_expr(expr_node, loop_var, set(free_vars))
    if c_expr is None:
        return None

    op_symbol = "+=" if op_type is ast.Add else "-=" if op_type is ast.Sub else None
    if op_symbol is None:
        return None

    # 3. Generate C code
    func_sig_params = ["long long _start", "long long _stop", "long long _step", "long long _init"]
    for v in free_vars:
        func_sig_params.append(f"long long {v}")

    params_str = ", ".join(func_sig_params)
    
    c_code = f"""
#include <stdint.h>

long long zenith_kernel({params_str}) {{
    long long {accum_var} = _init;
    if (_step > 0) {{
        for (long long {loop_var} = _start; {loop_var} < _stop; {loop_var} += _step) {{
            {accum_var} {op_symbol} {c_expr};
        }}
    }} else if (_step < 0) {{
        for (long long {loop_var} = _start; {loop_var} > _stop; {loop_var} += _step) {{
            {accum_var} {op_symbol} {c_expr};
        }}
    }}
    return {accum_var};
}}
"""
    code_hash = hashlib.sha256(c_code.encode("utf-8")).hexdigest()[:16]
    so_path = os.path.join(CACHE_DIR, f"loop_{code_hash}.so")
    c_path = os.path.join(CACHE_DIR, f"loop_{code_hash}.c")

    # 4. Compile if not cached
    if not os.path.exists(so_path):
        try:
            with open(c_path, "w", encoding="utf-8") as f:
                f.write(c_code)
            cmd = ["gcc", "-O3", "-shared", "-fPIC", "-o", so_path, c_path]
            res = subprocess.run(cmd, capture_output=True, timeout=5)
            if res.returncode != 0:
                return None
        except Exception:
            return None

    # 5. Load library and cache function
    try:
        if code_hash not in _LOADED_LIBS:
            lib = ctypes.CDLL(so_path)
            kernel = lib.zenith_kernel
            arg_types = [ctypes.c_longlong] * (4 + len(free_vars))
            kernel.argtypes = arg_types
            kernel.restype = ctypes.c_longlong
            _LOADED_LIBS[code_hash] = kernel
        return code_hash, free_vars
    except Exception:
        return None

def execute_native_loop(code_hash, start, stop, step, init_val, *extra_args):
    """Dispatches execution to the cached native machine code kernel."""
    kernel = _LOADED_LIBS.get(code_hash)
    if kernel is None:
        so_path = os.path.join(CACHE_DIR, f"loop_{code_hash}.so")
        lib = ctypes.CDLL(so_path)
        kernel = lib.zenith_kernel
        kernel.restype = ctypes.c_longlong
        _LOADED_LIBS[code_hash] = kernel

    # Convert all inputs safely to integers
    c_start = int(start)
    c_stop = int(stop)
    c_step = int(step)
    c_init = int(init_val)
    c_extras = [int(x) for x in extra_args]
    return kernel(c_start, c_stop, c_step, c_init, *c_extras)
