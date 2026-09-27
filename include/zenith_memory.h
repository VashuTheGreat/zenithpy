#ifndef ZENITH_MEMORY_H
#define ZENITH_MEMORY_H

#include "zenith.h"

#define ZENITH_ARENA_BLOCK_SIZE (1024 * 1024) /* 1 MB blocks */

typedef struct ZenithBlock {
    struct ZenithBlock* next;
    size_t capacity;
    size_t used;
    uint8_t data[];
} ZenithBlock;

typedef struct ZenithArena {
    ZenithBlock* head;
    ZenithBlock* current;
    size_t total_allocated;
} ZenithArena;

ZenithArena* zenith_arena_new(void);
void* zenith_arena_alloc(ZenithArena* arena, size_t size);
void zenith_arena_reset(ZenithArena* arena);
void zenith_arena_free(ZenithArena* arena);

/* Tracked GC memory manager for dynamic lifetime objects */
typedef struct ZenithHeap {
    ZenithArena* arena;
    struct ZenithHeader* objects;
    size_t bytes_allocated;
    size_t next_gc_threshold;
} ZenithHeap;

ZenithHeap* zenith_heap_new(void);
void* zenith_heap_alloc_object(ZenithHeap* heap, size_t size, int type);
void zenith_heap_free(ZenithHeap* heap);

#endif /* ZENITH_MEMORY_H */
