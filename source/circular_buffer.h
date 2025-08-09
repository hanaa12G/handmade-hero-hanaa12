#ifndef CIRCULAR_BUFFER_H_
#define CIRCULAR_BUFFER_H_

#include <cstddef>


struct circular_buffer {
    int ReadIndex = 0;
    int WriteIndex = 0;
    void* Memory = NULL;
    int   Size = 0;
};

void CircularBufferInit(circular_buffer*,
                              void* Memory,
                              int MemorySize);

/// Read `Size` bytes from buffer to `Memory`
void CircularBufferRead(circular_buffer*, void* Memory, int Size);
/// Write `Size` bytes from `Memory` to buffer
void CircularBufferWrite(circular_buffer*, void const* Memory, int Size);
int  CircularBufferReadableSize(circular_buffer const*);
int  CircularBufferWritableSize(circular_buffer const*);


#endif
