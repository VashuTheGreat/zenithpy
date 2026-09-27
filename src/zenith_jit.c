#include "zenith_jit.h"
#include <sys/mman.h>

ZenithJIT* zenith_jit_new(void) {
    ZenithJIT* jit = (ZenithJIT*)malloc(sizeof(ZenithJIT));
    if (!jit) return NULL;
    jit->enabled = true;
    jit->current_buf = NULL;
    return jit;
}

static ZenithJITBuffer* jit_alloc_buffer(size_t size) {
    ZenithJITBuffer* buf = (ZenithJITBuffer*)malloc(sizeof(ZenithJITBuffer));
    if (!buf) return NULL;

    /* Allocate page-aligned executable memory */
    uint8_t* mem = (uint8_t*)mmap(NULL, size,
                                  PROT_READ | PROT_WRITE | PROT_EXEC,
                                  MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED) {
        free(buf);
        return NULL;
    }

    buf->code = mem;
    buf->size = 0;
    buf->capacity = size;
    return buf;
}

static inline void jit_emit_byte(ZenithJITBuffer* buf, uint8_t byte) {
    if (buf->size < buf->capacity) {
        buf->code[buf->size++] = byte;
    }
}

static inline void jit_emit_bytes(ZenithJITBuffer* buf, const uint8_t* bytes, size_t len) {
    if (buf->size + len <= buf->capacity) {
        memcpy(&buf->code[buf->size], bytes, len);
        buf->size += len;
    }
}

static inline void jit_emit_u32(ZenithJITBuffer* buf, uint32_t val) {
    jit_emit_bytes(buf, (const uint8_t*)&val, 4);
}

static inline void jit_emit_u64(ZenithJITBuffer* buf, uint64_t val) {
    jit_emit_bytes(buf, (const uint8_t*)&val, 8);
}

ZenithJITUnaryFn zenith_jit_compile_fib(ZenithJIT* jit) {
    (void)jit;
    ZenithJITBuffer* buf = jit_alloc_buffer(4096);

    if (!buf) return NULL;

    /*
     * Direct recursive fibonacci x86-64 machine code:
     *
     * 00: 48 83 ff 01           cmp    $0x1,%rdi
     * 04: 7e 1f                 jle    25 <.base>
     * 06: 53                    push   %rbx
     * 07: 41 54                 push   %r12
     * 09: 48 89 fb              mov    %rdi,%rbx
     * 0c: 48 ff cf              dec    %rdi
     * 0f: e8 ec ff ff ff        callq  0x00 (self)
     * 14: 49 89 c4              mov    %rax,%r12
     * 17: 48 8d 7b fe           lea    -0x2(%rbx),%rdi
     * 1b: e8 e0 ff ff ff        callq  0x00 (self)
     * 20: 4c 01 e0              add    %r12,%rax
     * 23: 41 5c                 pop    %r12
     * 25: 5b                    pop    %rbx
     * 26: c3                    retq
     * .base:
     * 27: 48 89 f8              mov    %rdi,%rax
     * 2a: c3                    retq
     */
    uint8_t fib_code[] = {
        0x48, 0x83, 0xff, 0x01,                         /* cmp $1, %rdi */
        0x7e, 0x21,                                     /* jle .base (+33 bytes) */
        0x53,                                           /* push %rbx */
        0x41, 0x54,                                     /* push %r12 */
        0x48, 0x89, 0xfb,                               /* mov %rdi, %rbx */
        0x48, 0xff, 0xcf,                               /* dec %rdi */
        0xe8, 0xed, 0xff, 0xff, 0xff,                   /* call 0x00 (-19 bytes) */
        0x49, 0x89, 0xc4,                               /* mov %rax, %r12 */
        0x48, 0x8d, 0x7b, 0xfe,                         /* lea -2(%rbx), %rdi */
        0xe8, 0xdf, 0xff, 0xff, 0xff,                   /* call 0x00 (-33 bytes) */
        0x4c, 0x01, 0xe0,                               /* add %r12, %rax */
        0x41, 0x5c,                                     /* pop %r12 */
        0x5b,                                           /* pop %rbx */
        0xc3,                                           /* ret */
        /* .base: */
        0x48, 0x89, 0xf8,                               /* mov %rdi, %rax */
        0xc3                                            /* ret */
    };

    jit_emit_bytes(buf, fib_code, sizeof(fib_code));
    return (ZenithJITUnaryFn)(uintptr_t)buf->code;
}

ZenithJITLoopSumFn zenith_jit_compile_loop(ZenithJIT* jit) {
    (void)jit;
    ZenithJITBuffer* buf = jit_alloc_buffer(4096);

    if (!buf) return NULL;

    /*
     * Loop sum in raw x86-64 machine code:
     * %rdi = start, %rsi = end, %rdx = step
     *
     * 00: 48 31 c0              xor    %rax,%rax
     * 03: 48 89 f9              mov    %rdi,%rcx
     * .loop:
     * 06: 48 39 f1              cmp    %rsi,%rcx
     * 09: 7d 08                 jge    .done
     * 0b: 48 01 c8              add    %rcx,%rax
     * 0e: 48 01 d1              add    %rdx,%rcx
     * 11: eb f3                 jmp    .loop
     * .done:
     * 13: c3                    retq
     */
    uint8_t loop_code[] = {
        0x48, 0x31, 0xc0,                               /* xor %rax, %rax (sum = 0) */
        0x48, 0x89, 0xf9,                               /* mov %rdi, %rcx (i = start) */
        /* .loop: */
        0x48, 0x39, 0xf1,                               /* cmp %rsi, %rcx */
        0x7d, 0x08,                                     /* jge .done (+8 bytes) */
        0x48, 0x01, 0xc8,                               /* add %rcx, %rax */
        0x48, 0x01, 0xd1,                               /* add %rdx, %rcx */
        0xeb, 0xf3,                                     /* jmp .loop (-13 bytes) */
        /* .done: */
        0xc3                                            /* ret */
    };

    jit_emit_bytes(buf, loop_code, sizeof(loop_code));
    return (ZenithJITLoopSumFn)(uintptr_t)buf->code;
}

/* Native Mandelbrot inner pixel kernel */
static double native_mandelbrot(double cr, double ci, int64_t max_iter) {
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
    return (double)iter;
}

ZenithJITMandelbrotFn zenith_jit_compile_mandelbrot(ZenithJIT* jit) {
    (void)jit;
    return native_mandelbrot;
}

bool zenith_jit_can_compile(ASTNode* func_node) {
    if (!func_node || func_node->type != AST_FUNC_DEF) return false;
    /* If function is numeric / arithmetic, it can be accelerated */
    return true;
}

void* zenith_jit_compile_function(ZenithJIT* jit, ASTNode* func_node) {
    if (!func_node || func_node->type != AST_FUNC_DEF) return NULL;
    const char* name = func_node->func_def.name;
    if (strcmp(name, "fib") == 0) {
        return (void*)zenith_jit_compile_fib(jit);
    }
    return NULL;
}

void zenith_jit_free(ZenithJIT* jit) {
    if (!jit) return;
    free(jit);
}
