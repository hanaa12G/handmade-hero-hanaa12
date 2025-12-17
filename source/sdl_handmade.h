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

struct sdl_game_code {
    SDL_SharedObject* Object = NULL;
    SDL_Time DllLastWriteTime;
    
    game_update_and_render* GameUpdateAndRender = &GameUpdateAndRenderStub;
    game_get_sound_sample* GameGetSoundSample = &GameGetSoundSampleStub;

    bool IsValid = false;
};

struct sdl_record_state
{
    uint64_t      MemoryStartAddress = 0;
    uint64_t      MemorySize = 0;
    void*         MemoryBlock = NULL;
    SDL_IOStream* RecordingStream = NULL;
    SDL_IOStream* PlaybackStream  = NULL;
    int RecordingIndex = 0;
    int PlaybackIndex = 0;
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

    sdl_game_code GameCode = {};
    char* SourceGameCodeDllFullPath = NULL;
    char* TmpGameCodeDllFullPath = NULL;


    SDL_Storage* UserStorage = NULL;
    SDL_Storage* GameStorage = NULL;

    sdl_record_state RecordingState;
};




#endif
