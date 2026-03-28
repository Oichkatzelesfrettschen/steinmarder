// sm_wavefront.c
#include "sm_wavefront.h"
#include <stdlib.h>
#include <string.h>

void sm_queue_init(SM_PathQueue* q, uint32_t capacity) {
    q->items = (SM_Path*)calloc((size_t)capacity, sizeof(SM_Path));
    q->count = 0;
    q->capacity = capacity;
}

void sm_queue_free(SM_PathQueue* q) {
    free(q->items);
    q->items = NULL;
    q->count = 0;
    q->capacity = 0;
}

void sm_queue_clear(SM_PathQueue* q) { q->count = 0; }

int sm_queue_push(SM_PathQueue* q, const SM_Path* p) {
    if (q->count >= q->capacity) return 0;
    q->items[q->count++] = *p;
    return 1;
}

int sm_wavefront_init(SM_WavefrontState* st, uint32_t max_paths) {
    memset(st, 0, sizeof(*st));
    sm_queue_init(&st->q_active, max_paths);
    sm_queue_init(&st->q_next,   max_paths);
    st->hits = (SM_SurfHit*)calloc((size_t)max_paths, sizeof(SM_SurfHit));
    return st->hits != NULL;
}

void sm_wavefront_free(SM_WavefrontState* st) {
    sm_queue_free(&st->q_active);
    sm_queue_free(&st->q_next);
    free(st->hits);
    st->hits = NULL;
}

void sm_wavefront_render(const SM_WavefrontSettings* s,
                          SM_WavefrontState* st,
                          SM_IntersectCB intersect_cb,
                          SM_ShadeCB shade_cb,
                          void* user)
{
    // This does not generate rays itself.
    // You will wire:
    //   - per-tile or per-thread primary ray generation into st->q_active
    //   - then call this function per tile / per batch

    for (uint32_t bounce = 0; bounce < s->max_depth; ++bounce) {
        uint32_t n = st->q_active.count;
        if (n == 0) break;

        // 1) Intersect (BVH / packets / etc)
        intersect_cb(st->q_active.items, n, st->hits, user);

        // 2) Shade / scatter (your materials + RR + throughput update)
        sm_queue_clear(&st->q_next);
        shade_cb(st->q_active.items, st->hits, n, &st->q_next, user);

        // 3) Swap queues
        SM_PathQueue tmp = st->q_active;
        st->q_active = st->q_next;
        st->q_next = tmp;
    }
}
