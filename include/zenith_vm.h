#ifndef ZENITH_VM_H
#define ZENITH_VM_H

#include "zenith.h"
#include "zenith_value.h"
#include "zenith_ast.h"
#include "zenith_memory.h"

#define ZENITH_MAX_LOCALS 256
#define ZENITH_ENV_CAPACITY 64

typedef struct ZenithBinding {
    const char* name;
    ZenithValue value;
} ZenithBinding;

typedef struct ZenithEnv {
    struct ZenithEnv* parent;
    ZenithBinding bindings[ZENITH_ENV_CAPACITY];
    size_t count;
} ZenithEnv;


typedef struct ZenithFunction {
    ZenithHeader header;
    char* name;
    ASTNode* func_def_node;
    ZenithEnv* closure_env;
    /* Native compiled JIT code pointer if available */
    ZenithValue (*jit_code)(ZenithValue* args, size_t argc);
} ZenithFunction;

typedef ZenithValue (*ZenithBuiltinFn)(size_t argc, ZenithValue* args);

typedef struct ZenithBuiltin {
    ZenithHeader header;
    char* name;
    ZenithBuiltinFn fn;
} ZenithBuiltin;

typedef struct ZenithVM {
    ZenithArena* arena;
    ZenithHeap* heap;
    ZenithEnv* global_env;
    int call_depth;
    bool had_error;
    char error_msg[256];
} ZenithVM;

ZenithVM* zenith_vm_new(void);
void zenith_vm_free(ZenithVM* vm);

ZenithEnv* zenith_env_new(ZenithVM* vm, ZenithEnv* parent);
void zenith_env_set(ZenithEnv* env, const char* name, ZenithValue val);
bool zenith_env_get(ZenithEnv* env, const char* name, ZenithValue* out_val);

void zenith_register_builtin(ZenithVM* vm, const char* name, ZenithBuiltinFn fn);

/* Execution */
ZenithValue zenith_eval_expr(ZenithVM* vm, ZenithEnv* env, ASTNode* node);
ZenithResult zenith_exec_stmt(ZenithVM* vm, ZenithEnv* env, ASTNode* node, ZenithValue* return_val, bool* returned, bool* broke, bool* continued);
ZenithResult zenith_run(ZenithVM* vm, ASTNode* root);

/* High-speed assembly declarations */
extern ZenithValue zenith_asm_add(ZenithValue a, ZenithValue b);
extern ZenithValue zenith_asm_sub(ZenithValue a, ZenithValue b);
extern ZenithValue zenith_asm_mul(ZenithValue a, ZenithValue b);
extern int64_t zenith_asm_loop_sum(int64_t count);
extern double zenith_asm_vector_dot_avx2(const double* a, const double* b, size_t n);
extern int64_t zenith_asm_fib(int64_t n);
extern ZenithValue zenith_asm_invoke_jit(void* code_ptr, void* env, void* arg1, void* arg2);

#endif /* ZENITH_VM_H */
