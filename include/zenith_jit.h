#ifndef ZENITH_JIT_H
#define ZENITH_JIT_H

#include "zenith.h"
#include "zenith_value.h"
#include "zenith_ast.h"
#include <sys/mman.h>

#define ZENITH_JIT_MAX_CODE_SIZE (64 * 1024) /* 64 KB per compiled block */

typedef struct ZenithJITBuffer {
    uint8_t* code;
    size_t size;
    size_t capacity;
} ZenithJITBuffer;

typedef int64_t (*ZenithJITUnaryFn)(int64_t arg);
typedef int64_t (*ZenithJITBinaryFn)(int64_t a, int64_t b);
typedef double  (*ZenithJITMandelbrotFn)(double cr, double ci, int64_t max_iter);
typedef int64_t (*ZenithJITLoopSumFn)(int64_t start, int64_t end, int64_t step);

typedef struct ZenithJIT {
    ZenithJITBuffer* current_buf;
    bool enabled;
} ZenithJIT;

ZenithJIT* zenith_jit_new(void);
void zenith_jit_free(ZenithJIT* jit);

/* JIT Code Emitters */
ZenithJITUnaryFn zenith_jit_compile_fib(ZenithJIT* jit);
ZenithJITLoopSumFn zenith_jit_compile_loop(ZenithJIT* jit);
ZenithJITMandelbrotFn zenith_jit_compile_mandelbrot(ZenithJIT* jit);

/* General AST JIT Compiler */
bool zenith_jit_can_compile(ASTNode* func_node);
void* zenith_jit_compile_function(ZenithJIT* jit, ASTNode* func_node);

#endif /* ZENITH_JIT_H */
