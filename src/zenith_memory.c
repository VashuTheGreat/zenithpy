#include "zenith_memory.h"
#include "zenith_value.h"

ZenithArena* zenith_arena_new(void) {
    ZenithArena* arena = (ZenithArena*)malloc(sizeof(ZenithArena));
    if (!arena) return NULL;

    size_t block_sz = ZENITH_ARENA_BLOCK_SIZE;
    ZenithBlock* block = (ZenithBlock*)malloc(sizeof(ZenithBlock) + block_sz);
    if (!block) {
        free(arena);
        return NULL;
    }
    block->next = NULL;
    block->capacity = block_sz;
    block->used = 0;

    arena->head = block;
    arena->current = block;
    arena->total_allocated = sizeof(ZenithArena) + sizeof(ZenithBlock) + block_sz;
    return arena;
}

void* zenith_arena_alloc(ZenithArena* arena, size_t size) {
    if (!arena) return NULL;

    /* 8-byte alignment */
    size = (size + 7) & ~7;

    ZenithBlock* cur = arena->current;
    if (cur->used + size <= cur->capacity) {
        void* ptr = &cur->data[cur->used];
        cur->used += size;
        return ptr;
    }

    /* Allocate new block */
    size_t block_sz = ZENITH_ARENA_BLOCK_SIZE;
    if (size > block_sz) {
        block_sz = size + 1024;
    }

    ZenithBlock* new_block = (ZenithBlock*)malloc(sizeof(ZenithBlock) + block_sz);
    if (!new_block) return NULL;

    new_block->next = NULL;
    new_block->capacity = block_sz;
    new_block->used = size;

    cur->next = new_block;
    arena->current = new_block;
    arena->total_allocated += sizeof(ZenithBlock) + block_sz;

    return (void*)&new_block->data[0];
}

void zenith_arena_reset(ZenithArena* arena) {
    if (!arena) return;
    ZenithBlock* b = arena->head;
    while (b) {
        b->used = 0;
        b = b->next;
    }
    arena->current = arena->head;
}

void zenith_arena_free(ZenithArena* arena) {
    if (!arena) return;
    ZenithBlock* b = arena->head;
    while (b) {
        ZenithBlock* next = b->next;
        free(b);
        b = next;
    }
    free(arena);
}

ZenithHeap* zenith_heap_new(void) {
    ZenithHeap* heap = (ZenithHeap*)malloc(sizeof(ZenithHeap));
    if (!heap) return NULL;
    heap->arena = zenith_arena_new();
    heap->objects = NULL;
    heap->bytes_allocated = 0;
    heap->next_gc_threshold = 1024 * 1024 * 64; /* 64 MB */
    return heap;
}

void* zenith_heap_alloc_object(ZenithHeap* heap, size_t size, int type) {
    /* Fast path: allocate in bump arena */
    void* ptr = zenith_arena_alloc(heap->arena, size);
    if (!ptr) return NULL;

    ZenithHeader* hdr = (ZenithHeader*)ptr;
    hdr->type = (ZenithObjType)type;
    hdr->flags = 0;
    hdr->next = heap->objects;
    heap->objects = hdr;
    heap->bytes_allocated += size;

    return ptr;
}

void zenith_heap_free(ZenithHeap* heap) {
    if (!heap) return;
    zenith_arena_free(heap->arena);
    free(heap);
}
