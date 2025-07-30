#ifndef SDL_HANDMADE_H_
#define SDL_HANDMADE_H_

#include <cstdint>

#include "handmade.hpp"

struct sdl_application_state {
    bool AppIsRunning = true;
    bool GlobalPause  = false;
    bool IsSoundValid = false;
    uint64_t PerformanceCounterFreq = 0;

    game_inputs NewInputs = {};
    game_inputs OldInputs = {};

    SDL_KeyboardEvent* KeyboardEvents = NULL;
    int        KeyboardEventsCount = 0;
};

#endif
