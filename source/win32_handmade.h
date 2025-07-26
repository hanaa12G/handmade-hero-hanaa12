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
    /// Number of samples per second
    int       SamplesPerSecond;
    /// The current sample we're outputing/playing
    uint32_t  RunningSampleIndex;
    uint32_t  RunningSampleByte;
    /// Bytes per sample
    int       BytesPerSample;
    /// Size of audio buffer
    uint32_t  SecondaryBufferSize;
    float     tSine;
    /// Number of samples we want to write each frame
    ///
    /// It's called latency because we don't want to write only 1 frame worth
    /// of sample, instead we add some redundant to avoid slow frame
    int       LatencySampleCount;
};

struct win32_debug_time_marker {
    /// Cursor positions when we beginning to calculate and output sound
    DWORD OutputPlayCursor;
    DWORD OutputWriteCursor;

    DWORD OutputLocation;
    DWORD OutputByteCount;

    /// Cursor position when we flip 
    DWORD FlipPlayCursor;
    DWORD FlipWriteCursor;
};


#endif // WIN32_HANDMADE_H
