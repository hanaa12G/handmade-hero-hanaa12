#ifndef WIN32_HANDMADE_H

struct win32_offscreen_buffer {
    BITMAPINFO BitMapInfo;
    void*      Memory;
    int        Width;
    int        Height;
    int        Pitch;
    int        BytesPerPixel;
};

struct win32_window_dimension {
    int Width;
    int Height;
};

struct win32_sound_output {
    int       SamplesPerSecond;
    uint32_t  RunningSampleIndex;
    int       BytesPerSample;
    int       SecondaryBufferSize;
    float     tSine;
    int       LatencySampleCount;
};


#endif // WIN32_HANDMADE_H
