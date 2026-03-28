#ifndef SM_SHM_PORTAL_H
#define SM_SHM_PORTAL_H

#include <stdint.h>

// The core data structure for steinmarder audio/data streaming
typedef struct {
    float *buffer;
    int head;
    int tail;
    int size;
} RingBuffer;

// Function prototype for the producer
void sm_produce_safe(RingBuffer *rb, float sample);

#endif
