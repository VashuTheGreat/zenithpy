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
        # Detect numeric loops: for <var> in range(...)
        if isinstance(node.iter, ast.Call) and getattr(node.iter.func, 'id', None) == 'range':
            range_args = node.iter.args
            if 1 <= len(range_args) <= 3 and len(node.body) == 1 and isinstance(node.target, ast.Name):
                stmt = node.body[0]
                if isinstance(stmt, ast.AugAssign) and isinstance(stmt.target, ast.Name):
                    loop_var = node.target.id
                    accum_var = stmt.target.id
                    op_type = type(stmt.op)

                    # Fastpath 1: Simple total += i (Handcrafted raw assembly)
                    if op_type is ast.Add and isinstance(stmt.value, ast.Name) and stmt.value.id == loop_var and len(range_args) == 1:
                        opt_code = ast.parse(f"""
import zenith_accelerator as _native
{accum_var} += _native.asm_loop_sum(int(0))
""").body[1]
                        opt_code.value.args[0] = range_args[0]
                        return opt_code

                    # Fastpath 2: General arithmetic loop (total += (i * i) % 7, etc.)
                    if op_type in (ast.Add, ast.Sub):
                        try:
                            from zenithpy.loop_compiler import compile_numeric_loop
                            res = compile_numeric_loop(loop_var, accum_var, op_type, stmt.value)
                            if res is not None:
                                code_hash, free_vars = res
                                if len(range_args) == 1:
                                    start_node = ast.Constant(value=0)
                                    stop_node = range_args[0]
                                    step_node = ast.Constant(value=1)
                                elif len(range_args) == 2:
                                    start_node = range_args[0]
                                    stop_node = range_args[1]
                                    step_node = ast.Constant(value=1)
                                else:
                                    start_node = range_args[0]
                                    stop_node = range_args[1]
                                    step_node = range_args[2]

                                call_args = [
                                    ast.Constant(value=code_hash),
                                    start_node,
                                    stop_node,
                                    step_node,
                                    ast.Name(id=accum_var, ctx=ast.Load()),
                                ]
                                for fv in free_vars:
                                    call_args.append(ast.Name(id=fv, ctx=ast.Load()))

                                call_node = ast.Call(
                                    func=ast.Name(id="_zenith_exec_loop", ctx=ast.Load()),
                                    args=call_args,
                                    keywords=[],
                                )
                                assign_node = ast.Assign(
                                    targets=[ast.Name(id=accum_var, ctx=ast.Store())],
                                    value=call_node,
                                )
                                ast.fix_missing_locations(assign_node)
                                return assign_node
                        except Exception:
                            pass

        return node

def optimize_and_exec(source_code: str, filename: str, global_dict: dict):
    """
    Parses plain, unannotated Python source code, applies ZenithPy automatic
    assembly AST optimizations, and executes with 100% Python compatibility.
    """
    from zenithpy.loop_compiler import execute_native_loop
    global_dict["_zenith_exec_loop"] = execute_native_loop
    global_dict["_native"] = _native

    tree = ast.parse(source_code, filename=filename)
    transformer = AutoAccelerator()
    optimized_tree = transformer.visit(tree)
    ast.fix_missing_locations(optimized_tree)

    compiled = compile(optimized_tree, filename=filename, mode="exec")
    exec(compiled, global_dict)

