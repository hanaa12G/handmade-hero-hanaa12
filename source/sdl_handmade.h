#ifndef SDL_HANDMADE_H_
#define SDL_HANDMADE_H_

#include <cstdint>

#include "handmade.hpp"
#include "circular_buffer.h"

struct sdl_offscreen_buffer;

struct sdl_offscreen_buffer
{
    int Width = 0;
    int Height = 0;
    int Pitch = 0;
    int BytesPerPixel = 0;
    SDL_Surface* Surface = NULL;
};

struct sdl_sound_output {
    uint64_t RunningSampleIndex = 0;
    uint64_t QueuedSampleIndex = 0;
    void* Memory = NULL;
    int BytesPerSample = sizeof(int16_t) * 2;
    circular_buffer Buffer = {};
};

struct sdl_application_state {
    bool AppIsRunning = true;
    bool GlobalPause  = false;
    bool IsSoundValid = false;

    SDL_KeyboardEvent* KeyboardEvents = NULL;
    int        KeyboardEventsCount = 0;


    sdl_offscreen_buffer Buffer = {};

    game_inputs NewInputs = {};
    game_inputs OldInputs = {};

    game_memory GameMemory = {};

    SDL_AudioStream* AudioStream = NULL;
    sdl_sound_output SoundOutput = {};

    int AudioAdditional = 0;
    int AudioTotal = 0;
};

#endif
