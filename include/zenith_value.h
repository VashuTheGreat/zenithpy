#ifndef ZENITH_VALUE_H
#define ZENITH_VALUE_H

#include "zenith.h"

/*
 * NaN-Boxing Scheme (64-bit unboxed value):
 * Top 16 bits define the tag if quiet NaN is set.
 *
 * IEEE-754 Quiet NaN: 0x7FF8_0000_0000_0000
 *
 * If (val & 0xFFF8_0000_0000_0000) != 0x7FF8_0000_0000_0000:
 *   -> IEEE-754 Double Precision Floating Point (64-bit float)
 *
 * If Quiet NaN bits are set:
 *   0x7FF8_0001_xxxx_xxxx -> 48-bit signed integer
 *   0x7FF8_0002_0000_0000 -> False
 *   0x7FF8_0002_0000_0001 -> True
 *   0x7FF8_0003_0000_0000 -> None
 *   0x7FF8_0004_xxxx_xxxx -> Heap Object Pointer (48-bit address)
 */

typedef uint64_t ZenithValue;

#define ZENITH_QNAN_MASK    0xFFF8000000000000ULL
#define ZENITH_QNAN_PREFIX  0x7FF8000000000000ULL

#define ZENITH_TAG_INT      0x0001000000000000ULL
#define ZENITH_TAG_BOOL     0x0002000000000000ULL
#define ZENITH_TAG_NONE     0x0003000000000000ULL
#define ZENITH_TAG_OBJ      0x0004000000000000ULL

#define ZENITH_TAG_MASK     0xFFFF000000000000ULL
#define ZENITH_PAYLOAD_MASK 0x0000FFFFFFFFFFFFULL

/* Value Constructors */
static ZENITH_ALWAYS_INLINE ZenithValue zenith_val_float(double d) {
    union { double f; uint64_t u; } cvt;
    cvt.f = d;
    /* If d happens to be a NaN matching our tag prefix, normalize it */
    if (ZENITH_UNLIKELY((cvt.u & ZENITH_QNAN_MASK) == ZENITH_QNAN_PREFIX)) {
        cvt.u = 0x7FF8000000000000ULL; /* Canonical QNaN */
    }
    return cvt.u;
}

static ZENITH_ALWAYS_INLINE ZenithValue zenith_val_int(int64_t i) {
    return ZENITH_QNAN_PREFIX | ZENITH_TAG_INT | ((uint64_t)i & ZENITH_PAYLOAD_MASK);
}

static ZENITH_ALWAYS_INLINE ZenithValue zenith_val_bool(bool b) {
    return ZENITH_QNAN_PREFIX | ZENITH_TAG_BOOL | (b ? 1ULL : 0ULL);
}

static ZENITH_ALWAYS_INLINE ZenithValue zenith_val_none(void) {
    return ZENITH_QNAN_PREFIX | ZENITH_TAG_NONE;
}

static ZENITH_ALWAYS_INLINE ZenithValue zenith_val_obj(void* ptr) {
    return ZENITH_QNAN_PREFIX | ZENITH_TAG_OBJ | ((uintptr_t)ptr & ZENITH_PAYLOAD_MASK);
}

/* Value Type Checkers */
static ZENITH_ALWAYS_INLINE bool zenith_is_float(ZenithValue v) {
    return (v & ZENITH_QNAN_MASK) != ZENITH_QNAN_PREFIX;
}

static ZENITH_ALWAYS_INLINE bool zenith_is_int(ZenithValue v) {
    return (v & ZENITH_TAG_MASK) == (ZENITH_QNAN_PREFIX | ZENITH_TAG_INT);
}

static ZENITH_ALWAYS_INLINE bool zenith_is_number(ZenithValue v) {
    return zenith_is_float(v) || zenith_is_int(v);
}

static ZENITH_ALWAYS_INLINE bool zenith_is_bool(ZenithValue v) {
    return (v & ZENITH_TAG_MASK) == (ZENITH_QNAN_PREFIX | ZENITH_TAG_BOOL);
}

static ZENITH_ALWAYS_INLINE bool zenith_is_none(ZenithValue v) {
    return v == (ZENITH_QNAN_PREFIX | ZENITH_TAG_NONE);
}

static ZENITH_ALWAYS_INLINE bool zenith_is_obj(ZenithValue v) {
    return (v & ZENITH_TAG_MASK) == (ZENITH_QNAN_PREFIX | ZENITH_TAG_OBJ);
}

/* Value Extractors */
static ZENITH_ALWAYS_INLINE double zenith_as_float(ZenithValue v) {
    if (zenith_is_int(v)) {
        /* Convert int payload to double */
        int64_t val = (int64_t)(v & ZENITH_PAYLOAD_MASK);
        if (val & 0x0000800000000000ULL) { /* sign extension for 48-bit */
            val |= 0xFFFF000000000000ULL;
        }
        return (double)val;
    }
    union { uint64_t u; double f; } cvt;
    cvt.u = v;
    return cvt.f;
}

static ZENITH_ALWAYS_INLINE int64_t zenith_as_int(ZenithValue v) {
    if (zenith_is_float(v)) {
        union { uint64_t u; double f; } cvt;
        cvt.u = v;
        return (int64_t)cvt.f;
    }
    int64_t val = (int64_t)(v & ZENITH_PAYLOAD_MASK);
    if (val & 0x0000800000000000ULL) {
        val |= 0xFFFF000000000000ULL; /* sign extend */
    }
    return val;
}

static ZENITH_ALWAYS_INLINE bool zenith_as_bool(ZenithValue v) {
    if (zenith_is_bool(v)) {
        return (v & 1ULL) != 0;
    }
    if (zenith_is_int(v)) {
        return zenith_as_int(v) != 0;
    }
    if (zenith_is_float(v)) {
        return zenith_as_float(v) != 0.0;
    }
    if (zenith_is_none(v)) {
        return false;
    }
    return true; /* Objects are truthy unless empty */
}

static ZENITH_ALWAYS_INLINE void* zenith_as_obj(ZenithValue v) {
    return (void*)(uintptr_t)(v & ZENITH_PAYLOAD_MASK);
}

/* Heap Object Types */
typedef enum {
    ZENITH_OBJ_STRING,
    ZENITH_OBJ_LIST,
    ZENITH_OBJ_DICT,
    ZENITH_OBJ_FUNCTION,
    ZENITH_OBJ_NATIVE_FUNC
} ZenithObjType;

typedef struct ZenithHeader {
    ZenithObjType type;
    uint32_t flags;
    struct ZenithHeader* next; /* for GC tracking */
} ZenithHeader;

typedef struct ZenithString {
    ZenithHeader header;
    uint32_t length;
    uint32_t hash;
    char chars[];
} ZenithString;

typedef struct ZenithList {
    ZenithHeader header;
    uint32_t count;
    uint32_t capacity;
    ZenithValue* items;
} ZenithList;

typedef struct ZenithDictEntry {
    ZenithValue key;
    ZenithValue value;
    uint32_t hash;
    bool occupied;
} ZenithDictEntry;

typedef struct ZenithDict {
    ZenithHeader header;
    uint32_t count;
    uint32_t capacity;
    ZenithDictEntry* entries;
} ZenithDict;

/* Function signatures */
ZenithString* zenith_string_new(const char* str, size_t len);
ZenithList* zenith_list_new(size_t initial_cap);
void zenith_list_append(ZenithList* list, ZenithValue val);
ZenithValue zenith_list_get(ZenithList* list, int64_t index);
void zenith_list_set(ZenithList* list, int64_t index, ZenithValue val);

void zenith_value_print(ZenithValue v);
void zenith_value_println(ZenithValue v);
bool zenith_value_equal(ZenithValue a, ZenithValue b);

#endif /* ZENITH_VALUE_H */
