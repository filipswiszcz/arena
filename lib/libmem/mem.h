#ifndef __LIBMEM__
#define __LIBMEM__

#include <stddef.h>
#if defined(__cplusplus)
    #include <cstddef>
#endif
#include <stdint.h>
#include <stdio.h>

#if defined(__cplusplus)
    #define MEM_ARENA_DEFAULT_ALIGN alignof(std::max_align_t)
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L && !defined(_MSC_VER)
    #define MEM_ARENA_DEFAULT_ALIGN _Alignof(max_align_t)
#else
    #define MEM_ARENA_DEFAULT_ALIGN (2 * sizeof(void*))
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mem_arena {
    uint8_t *head;
    size_t used, capacity;
} mem_arena_t;

static inline void mem_arena_init(mem_arena_t *arena, void *head, size_t capacity) {
    arena->head = head;
    arena->used = 0;
    arena->capacity = capacity;
}

static inline void *mem_arena_alloc(mem_arena_t *arena, size_t size) {
    uintptr_t align = MEM_ARENA_DEFAULT_ALIGN;
    uintptr_t caddr = (uintptr_t) arena->head + arena->used;
    uintptr_t naddr = (caddr + align - 1) & ~(align - 1);

    size_t padding = naddr - caddr;
    size_t avail = arena->capacity - arena->used;
    if (avail < padding || (avail -= padding) < size) {
        printf("Arena out of memory!\n"); return NULL;
    }

    arena->used += (padding + size);

    return (void*) naddr;
}

static inline void mem_arena_free(mem_arena_t *arena) {
    arena->used = 0;
}

#ifdef __cplusplus
}
#endif

#endif // !__LIBMEM__