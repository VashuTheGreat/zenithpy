/*
 * ZenithPy - High Performance x86-64 Assembly Fastpaths
 * System V AMD64 ABI:
 *   Arguments: %rdi, %rsi, %rdx, %rcx, %r8, %r9
 *   Return:    %rax, %xmm0
 *   Callee-saved: %rbx, %rsp, %rbp, %r12, %r13, %r14, %r15
 */

.text
.globl zenith_asm_add
.globl zenith_asm_sub
.globl zenith_asm_mul
.globl zenith_asm_loop_sum
.globl zenith_asm_vector_dot_avx2
.globl zenith_asm_fib
.globl zenith_asm_invoke_jit

# Constants
# QNAN_PREFIX | TAG_INT = 0x7FF8000100000000
# QNAN_MASK             = 0xFFF8000000000000
# TAG_MASK              = 0xFFFF000000000000

.align 16
zenith_asm_add:
    # %rdi = a (ZenithValue)
    # %rsi = b (ZenithValue)
    movabsq $0xFFFF000000000000, %r8
    movabsq $0x7FF9000000000000, %r9

    movq %rdi, %rax
    movq %rsi, %rcx
    andq %r8, %rax
    andq %r8, %rcx

    # Check if both are integers
    cmpq %r9, %rax
    jne .Ladd_float
    cmpq %r9, %rcx
    jne .Ladd_float

    # Both are integers! Extract sign-extended 48-bit values
    # Sign extend from bit 47: shl $16, then sar $16
    movq %rdi, %rax
    shlq $16, %rax
    sarq $16, %rax

    movq %rsi, %rcx
    shlq $16, %rcx
    sarq $16, %rcx

    addq %rcx, %rax

    # Pack back into ZenithValue (tag + 48-bit mask)
    movabsq $0x0000FFFFFFFFFFFF, %rcx
    andq %rcx, %rax
    orq  %r9, %rax
    ret

.Ladd_float:
    # Fallback to float addition
    # If a is int, convert to float in %xmm0
    cmpq %r9, %rax
    jne .La_is_float
    # a is int
    movq %rdi, %rax
    shlq $16, %rax
    sarq $16, %rax
    cvtsi2sdq %rax, %xmm0
    jmp .Lcheck_b

.La_is_float:
    movq %rdi, %xmm0

.Lcheck_b:
    cmpq %r9, %rcx
    jne .Lb_is_float
    # b is int
    movq %rsi, %rcx
    shlq $16, %rcx
    sarq $16, %rcx
    cvtsi2sdq %rcx, %xmm1
    jmp .Ldo_float_add

.Lb_is_float:
    movq %rsi, %xmm1

.Ldo_float_add:
    addsd %xmm1, %xmm0
    movq %xmm0, %rax
    ret

.align 16
zenith_asm_sub:
    # %rdi = a, %rsi = b
    movabsq $0xFFFF000000000000, %r8
    movabsq $0x7FF9000000000000, %r9

    movq %rdi, %rax
    movq %rsi, %rcx
    andq %r8, %rax
    andq %r8, %rcx

    cmpq %r9, %rax
    jne .Lsub_float
    cmpq %r9, %rcx
    jne .Lsub_float

    # Fast integer subtraction
    movq %rdi, %rax
    shlq $16, %rax
    sarq $16, %rax

    movq %rsi, %rcx
    shlq $16, %rcx
    sarq $16, %rcx

    subq %rcx, %rax

    movabsq $0x0000FFFFFFFFFFFF, %rcx
    andq %rcx, %rax
    orq  %r9, %rax
    ret

.Lsub_float:
    cmpq %r9, %rax
    jne .Lsub_a_flt
    movq %rdi, %rax
    shlq $16, %rax
    sarq $16, %rax
    cvtsi2sdq %rax, %xmm0
    jmp .Lsub_check_b
.Lsub_a_flt:
    movq %rdi, %xmm0

.Lsub_check_b:
    cmpq %r9, %rcx
    jne .Lsub_b_flt
    movq %rsi, %rcx
    shlq $16, %rcx
    sarq $16, %rcx
    cvtsi2sdq %rcx, %xmm1
    jmp .Lsub_do_flt
.Lsub_b_flt:
    movq %rsi, %xmm1

.Lsub_do_flt:
    subsd %xmm1, %xmm0
    movq %xmm0, %rax
    ret

.align 16
zenith_asm_mul:
    # %rdi = a, %rsi = b
    movabsq $0xFFFF000000000000, %r8
    movabsq $0x7FF9000000000000, %r9

    movq %rdi, %rax
    movq %rsi, %rcx
    andq %r8, %rax
    andq %r8, %rcx

    cmpq %r9, %rax
    jne .Lmul_float
    cmpq %r9, %rcx
    jne .Lmul_float

    # Fast integer multiplication
    movq %rdi, %rax
    shlq $16, %rax
    sarq $16, %rax

    movq %rsi, %rcx
    shlq $16, %rcx
    sarq $16, %rcx

    imulq %rcx, %rax

    movabsq $0x0000FFFFFFFFFFFF, %rcx
    andq %rcx, %rax
    orq  %r9, %rax
    ret

.Lmul_float:
    cmpq %r9, %rax
    jne .Lmul_a_flt
    movq %rdi, %rax
    shlq $16, %rax
    sarq $16, %rax
    cvtsi2sdq %rax, %xmm0
    jmp .Lmul_check_b
.Lmul_a_flt:
    movq %rdi, %xmm0

.Lmul_check_b:
    cmpq %r9, %rcx
    jne .Lmul_b_flt
    movq %rsi, %rcx
    shlq $16, %rcx
    sarq $16, %rcx
    cvtsi2sdq %rcx, %xmm1
    jmp .Lmul_do_flt
.Lmul_b_flt:
    movq %rsi, %xmm1

.Lmul_do_flt:
    mulsd %xmm1, %xmm0
    movq %xmm0, %rax
    ret

.align 16
zenith_asm_fib:
    # int64_t zenith_asm_fib(int64_t n)
    # %rdi = n
    cmpq $1, %rdi
    jle .Lfib_base
    pushq %rbx
    pushq %r12
    movq %rdi, %rbx
    decq %rdi
    call zenith_asm_fib
    movq %rax, %r12
    leaq -2(%rbx), %rdi
    call zenith_asm_fib
    addq %r12, %rax
    popq %r12
    popq %rbx
    ret
.Lfib_base:
    movq %rdi, %rax
    ret

.align 16
zenith_asm_loop_sum:
    # int64_t zenith_asm_loop_sum(int64_t count)
    # %rdi = count
    xorq %rax, %rax
    xorq %rcx, %rcx
.Lloop_top:
    cmpq %rdi, %rcx
    jge .Lloop_done
    addq %rcx, %rax
    incq %rcx
    jmp .Lloop_top
.Lloop_done:
    ret

.align 16
zenith_asm_vector_dot_avx2:
    # double zenith_asm_vector_dot_avx2(const double* a, const double* b, size_t n)
    # %rdi = a, %rsi = b, %rdx = n
    vxorpd %ymm0, %ymm0, %ymm0
    vxorpd %ymm1, %ymm1, %ymm1
    xorq %rcx, %rcx

    # Process 8 doubles per iteration (2x 256-bit AVX registers)
.Lvec_loop:
    leaq 8(%rcx), %rax
    cmpq %rdx, %rax
    jg .Lvec_tail

    vmovupd (%rdi, %rcx, 8), %ymm2
    vmovupd (%rsi, %rcx, 8), %ymm3
    vfmadd231pd %ymm2, %ymm3, %ymm0

    vmovupd 32(%rdi, %rcx, 8), %ymm4
    vmovupd 32(%rsi, %rcx, 8), %ymm5
    vfmadd231pd %ymm4, %ymm5, %ymm1

    addq $8, %rcx
    jmp .Lvec_loop

.Lvec_tail:
    vaddpd %ymm1, %ymm0, %ymm0
    # Horizontal add ymm0
    vextractf128 $1, %ymm0, %xmm1
    vaddpd %xmm1, %xmm0, %xmm0
    vunpckhpd %xmm0, %xmm0, %xmm1
    vaddsd %xmm1, %xmm0, %xmm0

.Lscalar_loop:
    cmpq %rdx, %rcx
    jge .Lvec_done
    vmovsd (%rdi, %rcx, 8), %xmm2
    vmovsd (%rsi, %rcx, 8), %xmm3
    vfmadd231sd %xmm2, %xmm3, %xmm0
    incq %rcx
    jmp .Lscalar_loop

.Lvec_done:
    vzeroupper
    ret

.align 16
zenith_asm_invoke_jit:
    # ZenithValue zenith_asm_invoke_jit(void* fn_code, void* env)
    # %rdi = fn_code (function pointer)
    # %rsi = env (pointer to ZenithEnv or arguments)
    pushq %rbp
    movq %rsp, %rbp
    pushq %rbx
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15

    # %rdi = fn_code (function pointer)
    # %rsi = env / arg0
    # %rdx = arg1
    # %rcx = arg2
    movq %rdi, %r11
    movq %rsi, %rdi
    movq %rdx, %rsi
    movq %rcx, %rdx
    call *%r11

    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %rbx
    popq %rbp
    ret

.globl zenith_asm_prime_count
.align 16
zenith_asm_prime_count:
    # int64_t zenith_asm_prime_count(int64_t limit)
    # %rdi = limit
    pushq %rbx
    pushq %r12
    pushq %r13

    movq %rdi, %r12        # r12 = limit
    xorq %rbx, %rbx        # rbx = prime count
    movq $2, %r13          # r13 = candidate n

.Lprime_outer:
    cmpq %r12, %r13
    jg .Lprime_done

    movq $2, %rcx          # rcx = divisor d
.Lprime_inner:
    movq %rcx, %rax
    imulq %rcx, %rax       # rax = d * d
    cmpq %r13, %rax
    jg .Lprime_is_prime

    # Check n % d == 0
    movq %r13, %rax
    xorq %rdx, %rdx
    divq %rcx
    testq %rdx, %rdx
    jz .Lprime_not_prime

    incq %rcx
    jmp .Lprime_inner

.Lprime_is_prime:
    incq %rbx

.Lprime_not_prime:
    incq %r13
    jmp .Lprime_outer

.Lprime_done:
    movq %rbx, %rax
    popq %r13
    popq %r12
    popq %rbx
    ret

.globl zenith_asm_mandel_pixel
.align 16
zenith_asm_mandel_pixel:
    # int64_t zenith_asm_mandel_pixel(double cr, double ci, int64_t max_iter)
    # %xmm0 = cr, %xmm1 = ci, %rdi = max_iter
    xorpd %xmm2, %xmm2     # zr = 0.0
    xorpd %xmm3, %xmm3     # zi = 0.0
    movabsq $0x4010000000000000, %rax # 4.0 in IEEE 754
    movq %rax, %xmm4
    xorq %rax, %rax        # iter = 0

.Lmandel_loop:
    cmpq %rdi, %rax
    jge .Lmandel_done

    movapd %xmm2, %xmm5
    mulsd %xmm5, %xmm5     # zr2 = zr * zr

    movapd %xmm3, %xmm6
    mulsd %xmm6, %xmm6     # zi2 = zi * zi

    movapd %xmm5, %xmm7
    addsd %xmm6, %xmm7     # zr2 + zi2
    ucomisd %xmm4, %xmm7
    ja .Lmandel_done

    # zi = 2.0 * zr * zi + ci
    addsd %xmm3, %xmm3     # 2 * zi
    mulsd %xmm2, %xmm3     # 2 * zi * zr
    addsd %xmm1, %xmm3     # 2 * zi * zr + ci

    # zr = zr2 - zi2 + cr
    subsd %xmm6, %xmm5     # zr2 - zi2
    addsd %xmm0, %xmm5     # zr2 - zi2 + cr
    movapd %xmm5, %xmm2

    incq %rax
    jmp .Lmandel_loop

.Lmandel_done:
    ret


