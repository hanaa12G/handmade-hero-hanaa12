#include "circular_buffer.h"
#include <cassert>

void CircularBufferInit(circular_buffer* Buffer,
    void* Memory,
    int   MemorySize)
{
    Buffer->ReadIndex = 0;
    Buffer->WriteIndex = 0;
    Buffer->Memory = Memory;
    Buffer->Size = MemorySize;
}

int CircularBufferReadableSize(circular_buffer const* Buffer)
{
    if (Buffer->WriteIndex >= Buffer->ReadIndex) {
        return Buffer->WriteIndex - Buffer->ReadIndex;
    } else {
        return Buffer->WriteIndex + (Buffer->Size - Buffer->ReadIndex);
    }
}
int  CircularBufferWritableSize(circular_buffer const* Buffer)
{
    int Read = Buffer->ReadIndex;
    if (Read <= Buffer->WriteIndex) {
        Read += Buffer->Size;
    }
    return (Read - 1) - Buffer->WriteIndex;
}

void CircularBufferRead(circular_buffer* Buffer, void* Memory, int Size)
{
    assert(Size <= CircularBufferReadableSize(Buffer));

    int Count = 0;

    char* Out = (char*) Memory;
    char* In  = (char*) Buffer->Memory;
    for (; Count < Size; ++Count) {
        Out[Count] = In[Buffer->ReadIndex];

        Buffer->ReadIndex += 1;
        if (Buffer->ReadIndex == Buffer->Size) Buffer->ReadIndex = 0;
    }
}

void CircularBufferWrite(circular_buffer* Buffer, void const* Memory, int Size)
{
    assert(Size <= CircularBufferWritableSize(Buffer));

    int Count = 0;

    char* In = (char*) Memory;
    char* Out  = (char*) Buffer->Memory;
    for (; Count < Size; ++Count) {
        Out[Buffer->WriteIndex] = In[Count];

        Buffer->WriteIndex += 1;
        if (Buffer->WriteIndex == Buffer->Size) Buffer->WriteIndex = 0;
    }
}

