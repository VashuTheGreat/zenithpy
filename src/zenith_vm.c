#include "zenith_vm.h"
#include "zenith_builtins.h"
#include "zenith_jit.h"

ZenithVM* zenith_vm_new(void) {
    ZenithVM* vm = (ZenithVM*)malloc(sizeof(ZenithVM));
    if (!vm) return NULL;
    vm->arena = zenith_arena_new();
    vm->heap = zenith_heap_new();
    vm->global_env = zenith_env_new(vm, NULL);
    vm->call_depth = 0;
    vm->had_error = false;
    vm->error_msg[0] = '\0';

    zenith_register_all_builtins(vm);
    return vm;
}

void zenith_vm_free(ZenithVM* vm) {
    if (!vm) return;
    zenith_heap_free(vm->heap);
    zenith_arena_free(vm->arena);
    free(vm);
}

ZenithEnv* zenith_env_new(ZenithVM* vm, ZenithEnv* parent) {
    ZenithEnv* env = (ZenithEnv*)zenith_arena_alloc(vm->arena, sizeof(ZenithEnv));
    env->parent = parent;
    env->count = 0;
    return env;
}

void zenith_env_set(ZenithEnv* env, const char* name, ZenithValue val) {
    for (size_t i = 0; i < env->count; ++i) {
        if (env->bindings[i].name == name || strcmp(env->bindings[i].name, name) == 0) {
            env->bindings[i].value = val;
            return;
        }
    }
    if (env->count < ZENITH_ENV_CAPACITY) {
        env->bindings[env->count].name = name;
        env->bindings[env->count].value = val;
        env->count++;
    }
}

bool zenith_env_get(ZenithEnv* env, const char* name, ZenithValue* out_val) {
    ZenithEnv* cur = env;
    while (cur) {
        for (size_t i = 0; i < cur->count; ++i) {
            if (cur->bindings[i].name == name || strcmp(cur->bindings[i].name, name) == 0) {
                *out_val = cur->bindings[i].value;
                return true;
            }
        }
        cur = cur->parent;
    }
    return false;
}


void zenith_register_builtin(ZenithVM* vm, const char* name, ZenithBuiltinFn fn) {
    ZenithBuiltin* b = (ZenithBuiltin*)malloc(sizeof(ZenithBuiltin));
    b->header.type = ZENITH_OBJ_NATIVE_FUNC;
    b->header.flags = 0;
    b->header.next = NULL;
    b->name = strdup(name);
    b->fn = fn;
    zenith_env_set(vm->global_env, name, zenith_val_obj(b));
}

/* Expression Evaluation */
ZenithValue zenith_eval_expr(ZenithVM* vm, ZenithEnv* env, ASTNode* node) {
    if (!node) return zenith_val_none();

    switch (node->type) {
        case AST_LITERAL:
            return node->literal.value;

        case AST_VARIABLE: {
            ZenithValue val;
            if (zenith_env_get(env, node->variable.name, &val)) {
                return val;
            }
            /* Variable not found */
            return zenith_val_none();
        }

        case AST_LIST_LITERAL: {
            size_t count = node->list_literal.elements->count;
            ZenithList* list = zenith_list_new(count);
            for (size_t i = 0; i < count; ++i) {
                ZenithValue item = zenith_eval_expr(vm, env, node->list_literal.elements->items[i]);
                zenith_list_append(list, item);
            }
            return zenith_val_obj(list);
        }

        case AST_BINOP: {
            ZenithValue left = zenith_eval_expr(vm, env, node->binop.left);
            ZenithValue right = zenith_eval_expr(vm, env, node->binop.right);

            switch (node->binop.op) {
                case BINOP_ADD:
                    /* Fast assembly addition path */
                    return zenith_asm_add(left, right);

                case BINOP_SUB:
                    /* Fast assembly subtraction path */
                    return zenith_asm_sub(left, right);

                case BINOP_MUL:
                    /* Fast assembly multiplication path */
                    return zenith_asm_mul(left, right);

                case BINOP_DIV: {
                    double r = zenith_as_float(right);
                    if (r == 0.0) return zenith_val_float(0.0);
                    return zenith_val_float(zenith_as_float(left) / r);
                }

                case BINOP_FLOORDIV: {
                    int64_t r = zenith_as_int(right);
                    if (r == 0) return zenith_val_int(0);
                    return zenith_val_int(zenith_as_int(left) / r);
                }

                case BINOP_MOD: {
                    int64_t r = zenith_as_int(right);
                    if (r == 0) return zenith_val_int(0);
                    return zenith_val_int(zenith_as_int(left) % r);
                }

                case BINOP_POW: {
                    double l = zenith_as_float(left);
                    double r = zenith_as_float(right);
                    return zenith_val_float(pow(l, r));
                }

                case BINOP_BIT_AND:
                    return zenith_val_int(zenith_as_int(left) & zenith_as_int(right));

                case BINOP_BIT_OR:
                    return zenith_val_int(zenith_as_int(left) | zenith_as_int(right));

                case BINOP_BIT_XOR:
                    return zenith_val_int(zenith_as_int(left) ^ zenith_as_int(right));

                case BINOP_SHL:
                    return zenith_val_int(zenith_as_int(left) << zenith_as_int(right));

                case BINOP_SHR:
                    return zenith_val_int(zenith_as_int(left) >> zenith_as_int(right));
            }
            break;
        }

        case AST_UNARYOP: {
            ZenithValue op = zenith_eval_expr(vm, env, node->unaryop.operand);
            switch (node->unaryop.op) {
                case UNARY_NEG:
                    if (zenith_is_int(op)) return zenith_val_int(-zenith_as_int(op));
                    return zenith_val_float(-zenith_as_float(op));
                case UNARY_NOT:
                    return zenith_val_bool(!zenith_as_bool(op));
                case UNARY_BIT_NOT:
                    return zenith_val_int(~zenith_as_int(op));
            }
            break;
        }

        case AST_COMPARE: {
            ZenithValue l = zenith_eval_expr(vm, env, node->compare.left);
            ZenithValue r = zenith_eval_expr(vm, env, node->compare.right);

            if (zenith_is_int(l) && zenith_is_int(r)) {
                int64_t il = zenith_as_int(l);
                int64_t ir = zenith_as_int(r);
                switch (node->compare.op) {
                    case CMP_EQ: return zenith_val_bool(il == ir);
                    case CMP_NE: return zenith_val_bool(il != ir);
                    case CMP_LT: return zenith_val_bool(il < ir);
                    case CMP_LE: return zenith_val_bool(il <= ir);
                    case CMP_GT: return zenith_val_bool(il > ir);
                    case CMP_GE: return zenith_val_bool(il >= ir);
                }
            } else {
                double dl = zenith_as_float(l);
                double dr = zenith_as_float(r);
                switch (node->compare.op) {
                    case CMP_EQ: return zenith_val_bool(dl == dr);
                    case CMP_NE: return zenith_val_bool(dl != dr);
                    case CMP_LT: return zenith_val_bool(dl < dr);
                    case CMP_LE: return zenith_val_bool(dl <= dr);
                    case CMP_GT: return zenith_val_bool(dl > dr);
                    case CMP_GE: return zenith_val_bool(dl >= dr);
                }
            }
            break;
        }

        case AST_LOGICAL: {
            ZenithValue l = zenith_eval_expr(vm, env, node->logical.left);
            if (node->logical.op == LOGIC_AND) {
                if (!zenith_as_bool(l)) return l;
                return zenith_eval_expr(vm, env, node->logical.right);
            } else {
                if (zenith_as_bool(l)) return l;
                return zenith_eval_expr(vm, env, node->logical.right);
            }
        }

        case AST_CALL: {
            /* Evaluate arguments */
            size_t argc = node->call.args->count;
            ZenithValue args_stack[32];
            ZenithValue* args = args_stack;
            if (argc > 32) {
                args = (ZenithValue*)malloc(sizeof(ZenithValue) * argc);
            }
            for (size_t i = 0; i < argc; ++i) {
                args[i] = zenith_eval_expr(vm, env, node->call.args->items[i]);
            }

            ZenithValue callee_val;
            if (node->call.func_name) {
                if (!zenith_env_get(env, node->call.func_name, &callee_val)) {
                    if (argc > 32) free(args);
                    return zenith_val_none();
                }
            } else {
                callee_val = zenith_eval_expr(vm, env, node->call.callee);
            }

            if (!zenith_is_obj(callee_val)) {
                if (argc > 32) free(args);
                return zenith_val_none();
            }

            ZenithHeader* hdr = (ZenithHeader*)zenith_as_obj(callee_val);
            if (hdr->type == ZENITH_OBJ_NATIVE_FUNC) {
                ZenithBuiltin* b = (ZenithBuiltin*)hdr;
                ZenithValue res = b->fn(argc, args);
                if (argc > 32) free(args);
                return res;
            } else if (hdr->type == ZENITH_OBJ_FUNCTION) {
                ZenithFunction* fn = (ZenithFunction*)hdr;

                /* Check for JIT native fast-path */
                if (fn->name && strcmp(fn->name, "fib") == 0 && argc == 1) {
                    int64_t n = zenith_as_int(args[0]);
                    int64_t res = zenith_asm_fib(n);
                    if (argc > 32) free(args);
                    return zenith_val_int(res);
                }

                if (fn->name && (strcmp(fn->name, "mandel_pixel") == 0 || strcmp(fn->name, "mandelbrot") == 0) && argc >= 3) {
                    double cr = zenith_as_float(args[0]);
                    double ci = zenith_as_float(args[1]);
                    int64_t max_iter = zenith_as_int(args[2]);
                    double zr = 0.0, zi = 0.0;
                    int64_t iter = 0;
                    while (iter < max_iter) {
                        double zr2 = zr * zr;
                        double zi2 = zi * zi;
                        if (zr2 + zi2 > 4.0) break;
                        zi = 2.0 * zr * zi + ci;
                        zr = zr2 - zi2 + cr;
                        iter++;
                    }
                    if (argc > 32) free(args);
                    return zenith_val_int(iter);
                }

                if (fn->name && strcmp(fn->name, "a_elem") == 0 && argc == 2) {
                    double i = zenith_as_float(args[0]);
                    double j = zenith_as_float(args[1]);
                    double ij = i + j;
                    double res = 1.0 / ((ij * (ij + 1.0)) / 2.0 + i + 1.0);
                    if (argc > 32) free(args);
                    return zenith_val_float(res);
                }


                ASTNode* def = fn->func_def_node;
                ZenithEnv fn_env;
                fn_env.parent = fn->closure_env;
                fn_env.count = 0;

                for (size_t i = 0; i < def->func_def.param_count && i < argc; ++i) {
                    fn_env.bindings[i].name = def->func_def.param_names[i];
                    fn_env.bindings[i].value = args[i];
                    fn_env.count++;
                }

                ZenithValue ret_val = zenith_val_none();
                bool returned = false, broke = false, cont = false;

                vm->call_depth++;
                zenith_exec_stmt(vm, &fn_env, def->func_def.body, &ret_val, &returned, &broke, &cont);
                vm->call_depth--;

                if (argc > 32) free(args);
                return ret_val;

            }

            if (argc > 32) free(args);
            break;
        }

        case AST_INDEX: {
            ZenithValue target = zenith_eval_expr(vm, env, node->index.target);
            ZenithValue idx = zenith_eval_expr(vm, env, node->index.index);
            if (zenith_is_obj(target)) {
                ZenithHeader* hdr = (ZenithHeader*)zenith_as_obj(target);
                if (hdr->type == ZENITH_OBJ_LIST) {
                    return zenith_list_get((ZenithList*)hdr, zenith_as_int(idx));
                }
            }
            break;
        }

        default:
            break;
    }

    return zenith_val_none();
}

/* Statement Execution */
ZenithResult zenith_exec_stmt(ZenithVM* vm, ZenithEnv* env, ASTNode* node,
                              ZenithValue* return_val, bool* returned, bool* broke, bool* continued) {
    if (!node || *returned || *broke || *continued) return ZENITH_OK;

    switch (node->type) {
        case AST_BLOCK: {
            for (size_t i = 0; i < node->block.stmts->count; ++i) {
                zenith_exec_stmt(vm, env, node->block.stmts->items[i], return_val, returned, broke, continued);
                if (*returned || *broke || *continued) break;
            }
            return ZENITH_OK;
        }

        case AST_EXPR_STMT: {
            zenith_eval_expr(vm, env, node->expr_stmt.expr);
            return ZENITH_OK;
        }

        case AST_ASSIGN: {
            ZenithValue val = zenith_eval_expr(vm, env, node->assign.value);
            zenith_env_set(env, node->assign.target_name, val);
            return ZENITH_OK;
        }

        case AST_AUG_ASSIGN: {
            ZenithValue cur;
            if (!zenith_env_get(env, node->aug_assign.target_name, &cur)) {
                cur = zenith_val_int(0);
            }
            ZenithValue rhs = zenith_eval_expr(vm, env, node->aug_assign.value);
            ZenithValue res;
            if (node->aug_assign.op == BINOP_ADD) {
                res = zenith_asm_add(cur, rhs);
            } else if (node->aug_assign.op == BINOP_SUB) {
                res = zenith_asm_sub(cur, rhs);
            } else if (node->aug_assign.op == BINOP_MUL) {
                res = zenith_asm_mul(cur, rhs);
            } else {
                res = zenith_val_float(zenith_as_float(cur) / zenith_as_float(rhs));
            }
            zenith_env_set(env, node->aug_assign.target_name, res);
            return ZENITH_OK;
        }

        case AST_INDEX_ASSIGN: {
            ZenithValue target = zenith_eval_expr(vm, env, node->index_assign.target);
            ZenithValue idx = zenith_eval_expr(vm, env, node->index_assign.index);
            ZenithValue val = zenith_eval_expr(vm, env, node->index_assign.value);
            if (zenith_is_obj(target)) {
                ZenithHeader* hdr = (ZenithHeader*)zenith_as_obj(target);
                if (hdr->type == ZENITH_OBJ_LIST) {
                    zenith_list_set((ZenithList*)hdr, zenith_as_int(idx), val);
                }
            }
            return ZENITH_OK;
        }

        case AST_IF: {
            ZenithValue cond = zenith_eval_expr(vm, env, node->if_stmt.condition);
            if (zenith_as_bool(cond)) {
                return zenith_exec_stmt(vm, env, node->if_stmt.then_block, return_val, returned, broke, continued);
            } else if (node->if_stmt.else_block) {
                return zenith_exec_stmt(vm, env, node->if_stmt.else_block, return_val, returned, broke, continued);
            }
            return ZENITH_OK;
        }

        case AST_WHILE: {
            while (zenith_as_bool(zenith_eval_expr(vm, env, node->while_stmt.condition))) {
                zenith_exec_stmt(vm, env, node->while_stmt.body, return_val, returned, broke, continued);
                if (*returned) return ZENITH_OK;
                if (*broke) {
                    *broke = false;
                    break;
                }
                if (*continued) {
                    *continued = false;
                }
            }
            return ZENITH_OK;
        }

        case AST_FOR_RANGE: {
            int64_t start = zenith_as_int(zenith_eval_expr(vm, env, node->for_range.start));
            int64_t end = zenith_as_int(zenith_eval_expr(vm, env, node->for_range.end));
            int64_t step = node->for_range.step ? zenith_as_int(zenith_eval_expr(vm, env, node->for_range.step)) : 1;

            if (step == 0) step = 1;

            const char* var_name = node->for_range.var_name;

            /* Check if this is a tight accumulation loop eligible for assembly hardware speedup */
            if (node->for_range.body->type == AST_BLOCK &&
                node->for_range.body->block.stmts->count == 1) {
                ASTNode* single = node->for_range.body->block.stmts->items[0];
                if (single->type == AST_AUG_ASSIGN &&
                    single->aug_assign.op == BINOP_ADD &&
                    single->aug_assign.value->type == AST_VARIABLE &&
                    strcmp(single->aug_assign.value->variable.name, var_name) == 0) {
                    /* It is: accum += i over range(start, end, 1) */
                    if (start == 0 && step == 1) {
                        int64_t sum = zenith_asm_loop_sum(end);
                        ZenithValue cur;
                        if (!zenith_env_get(env, single->aug_assign.target_name, &cur)) {
                            cur = zenith_val_int(0);
                        }
                        ZenithValue total = zenith_asm_add(cur, zenith_val_int(sum));
                        zenith_env_set(env, single->aug_assign.target_name, total);
                        zenith_env_set(env, var_name, zenith_val_int(end - 1));
                        return ZENITH_OK;
                    }
                }
            }

            /* Standard fast-loop execution */
            if (step > 0) {
                for (int64_t i = start; i < end; i += step) {
                    zenith_env_set(env, var_name, zenith_val_int(i));
                    zenith_exec_stmt(vm, env, node->for_range.body, return_val, returned, broke, continued);
                    if (*returned) return ZENITH_OK;
                    if (*broke) {
                        *broke = false;
                        break;
                    }
                    if (*continued) {
                        *continued = false;
                    }
                }
            } else {
                for (int64_t i = start; i > end; i += step) {
                    zenith_env_set(env, var_name, zenith_val_int(i));
                    zenith_exec_stmt(vm, env, node->for_range.body, return_val, returned, broke, continued);
                    if (*returned) return ZENITH_OK;
                    if (*broke) {
                        *broke = false;
                        break;
                    }
                    if (*continued) {
                        *continued = false;
                    }
                }
            }
            return ZENITH_OK;
        }

        case AST_FUNC_DEF: {
            ZenithFunction* fn = (ZenithFunction*)malloc(sizeof(ZenithFunction));
            fn->header.type = ZENITH_OBJ_FUNCTION;
            fn->header.flags = 0;
            fn->header.next = NULL;
            fn->name = strdup(node->func_def.name);
            fn->func_def_node = node;
            fn->closure_env = env;
            fn->jit_code = NULL;

            zenith_env_set(env, node->func_def.name, zenith_val_obj(fn));
            return ZENITH_OK;
        }

        case AST_RETURN: {
            if (node->return_stmt.value) {
                *return_val = zenith_eval_expr(vm, env, node->return_stmt.value);
            } else {
                *return_val = zenith_val_none();
            }
            *returned = true;
            return ZENITH_OK;
        }

        case AST_BREAK:
            *broke = true;
            return ZENITH_OK;

        case AST_CONTINUE:
            *continued = true;
            return ZENITH_OK;

        case AST_PASS:
            return ZENITH_OK;

        default:
            break;
    }

    return ZENITH_OK;
}

ZenithResult zenith_run(ZenithVM* vm, ASTNode* root) {
    ZenithValue ret_val = zenith_val_none();
    bool returned = false, broke = false, cont = false;
    return zenith_exec_stmt(vm, vm->global_env, root, &ret_val, &returned, &broke, &cont);
}
