#ifndef ZENITH_H
#define ZENITH_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#define ZENITH_VERSION "0.1.0-alpha"

/* Compiler hints & performance attributes */
#if defined(__GNUC__) || defined(__clang__)
#define ZENITH_ALWAYS_INLINE inline __attribute__((always_inline))
#define ZENITH_NOINLINE __attribute__((noinline))
#define ZENITH_HOT __attribute__((hot))
#define ZENITH_COLD __attribute__((cold))
#define ZENITH_LIKELY(x) __builtin_expect(!!(x), 1)
#define ZENITH_UNLIKELY(x) __builtin_expect(!!(x), 0)
#define ZENITH_RESTRICT __restrict__
#else
#define ZENITH_ALWAYS_INLINE inline
#define ZENITH_NOINLINE
#define ZENITH_HOT
#define ZENITH_COLD
#define ZENITH_LIKELY(x) (x)
#define ZENITH_UNLIKELY(x) (x)
#define ZENITH_RESTRICT
#endif

/* Error codes */
typedef enum {
    ZENITH_OK = 0,
    ZENITH_ERR_SYNTAX,
    ZENITH_ERR_TYPE,
    ZENITH_ERR_NAME,
    ZENITH_ERR_INDEX,
    ZENITH_ERR_OVERFLOW,
    ZENITH_ERR_ZERO_DIVISION,
    ZENITH_ERR_RUNTIME,
    ZENITH_ERR_MEMORY
} ZenithResult;

#endif /* ZENITH_H */
