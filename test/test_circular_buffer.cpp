#include "source/circular_buffer.cpp"
#include <cstring>

int main(int Argc, char** Argv) {
    char Memory[4] = { };
    circular_buffer Buffer = {};
    CircularBufferInit(&Buffer, Memory, 4);

    assert(CircularBufferReadableSize(&Buffer) == 0);
    assert(CircularBufferWritableSize(&Buffer) == 3);

    char TestBuffer[4] = {};

    std::strncpy(TestBuffer, "ab", 2);

    CircularBufferWrite(&Buffer, TestBuffer, 2);
    assert(CircularBufferReadableSize(&Buffer) == 2);
    assert(CircularBufferWritableSize(&Buffer) == 3 - 2);

    std::memset(TestBuffer, '\0', sizeof(TestBuffer));

    CircularBufferRead(&Buffer, TestBuffer, 1);
    assert(std::strlen(TestBuffer) == 1);

    assert(0 == std::strcmp(TestBuffer, "a"));
    assert(CircularBufferWritableSize(&Buffer) ==  2);

    std::memset(TestBuffer, '\0', sizeof(TestBuffer));
    std::strncpy(TestBuffer, "de", 2);

    CircularBufferWrite(&Buffer, TestBuffer, 2);

    std::memset(TestBuffer, '\0', sizeof(TestBuffer));
    CircularBufferRead(&Buffer, TestBuffer, 3);
    assert(std::strlen(TestBuffer) == 3);

    assert(0 == std::strcmp(TestBuffer, "bde"));

    return 0;
}
