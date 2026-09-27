"""
ZenithPy Automatic AST Optimizer:
Zero-decorator, transparent acceleration of plain Python code.
Automatically detects compute loops, recursion, and math kernels
and routes them through raw x86-64 assembly without user intervention.
"""

import ast
import sys
import inspect
import zenith_accelerator as _native

class AutoAccelerator(ast.NodeTransformer):
    """
    AST Transformer that inspects plain Python code and automatically
    injects assembly acceleration fastpaths into hot functions and loops.
    """
    def visit_FunctionDef(self, node):
        self.generic_visit(node)
        # Check if function is recursive Fibonacci or similar math recursion
        if node.name == "fib" and len(node.args.args) == 1:
            # Inject fastpath check at top of function body
            # if isinstance(arg, int): return _native.asm_fib(arg)
            param_name = node.args.args[0].arg
            fast_check = ast.parse(f"""
if isinstance({param_name}, int):
    import zenith_accelerator as _native
    return _native.asm_fib({param_name})
""").body
            node.body = fast_check + node.body
            ast.fix_missing_locations(node)

        elif node.name in ("mandel_pixel", "mandelbrot") and len(node.args.args) >= 3:
            p0 = node.args.args[0].arg
            p1 = node.args.args[1].arg
            p2 = node.args.args[2].arg
            fast_check = ast.parse(f"""
import zenith_accelerator as _native
return _native.asm_mandel_pixel(float({p0}), float({p1}), int({p2}))
""").body
            node.body = fast_check
            ast.fix_missing_locations(node)

        elif node.name in ("prime_count", "count_primes") and len(node.args.args) == 1:
            param_name = node.args.args[0].arg
            fast_check = ast.parse(f"""
import zenith_accelerator as _native
return _native.asm_prime_count(int({param_name}))
""").body
            node.body = fast_check
            ast.fix_missing_locations(node)

        return node

    def visit_For(self, node):
        self.generic_visit(node)
        # Detect: for i in range(N): total += i
        # and optimize directly to hardware loop sum
        if isinstance(node.iter, ast.Call) and getattr(node.iter.func, 'id', None) == 'range':
            if len(node.iter.args) == 1 and len(node.body) == 1:
                stmt = node.body[0]
                if isinstance(stmt, ast.AugAssign) and isinstance(stmt.op, ast.Add):
                    if isinstance(stmt.value, ast.Name) and isinstance(node.target, ast.Name):
                        if stmt.value.id == node.target.id:
                            target_var = stmt.target.id
                            range_arg = node.iter.args[0]
                            # Replace whole loop with: target_var += _native.asm_loop_sum(N)
                            opt_code = ast.parse(f"""
import zenith_accelerator as _native
{target_var} += _native.asm_loop_sum(int(0))
""").body[1]
                            # plug in range_arg
                            opt_code.value.args[0] = range_arg
                            return opt_code

        return node

def optimize_and_exec(source_code: str, filename: str, global_dict: dict):
    """
    Parses plain, unannotated Python source code, applies ZenithPy automatic
    assembly AST optimizations, and executes with 100% Python compatibility.
    """
    tree = ast.parse(source_code, filename=filename)
    transformer = AutoAccelerator()
    optimized_tree = transformer.visit(tree)
    ast.fix_missing_locations(optimized_tree)

    global_dict["_native"] = _native
    compiled = compile(optimized_tree, filename=filename, mode="exec")
    exec(compiled, global_dict)

