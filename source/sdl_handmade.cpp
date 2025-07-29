#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "sdl_handmade.h"

#ifndef HANDMDE_GLOBAL_DEFINE
#define HANDMDE_GLOBAL_DEFINE

#define global_variable static
#define local_persist static
#define internal_func static

#endif

#define ARRAY_SIZE(x) (sizeof((x)) / sizeof((x)[0]))

global_variable SDL_Window* Window;
global_variable SDL_Renderer* Renderer;

SDL_AppResult SDL_AppInit(void** AppState, int argc, char* argv[])
{
    sdl_application_state* ApplicationState =  new sdl_application_state();
    ApplicationState->PerformanceCounterFreq =  SDL_GetPerformanceFrequency();


    *AppState = ApplicationState;

    SDL_SetAppMetadata("handmade-hero-hanaa12", "0.1", NULL);
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError()); return SDL_APP_FAILURE;
        return SDL_APP_FAILURE;
    } if (!SDL_CreateWindowAndRenderer("Handmade Hero", 640, 480, 0, &Window, &Renderer)) {
        SDL_Log("Coundn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* AppState, SDL_Event* event)
{
    sdl_application_state* ApplicationState = (sdl_application_state*) AppState;


    switch (event->type) {
        case SDL_EVENT_QUIT:
        {
            return SDL_APP_SUCCESS;
        } break;
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* AppState)
{
    sdl_application_state* ApplicationState = (sdl_application_state*) AppState;

    game_inputs* NewInputs = &ApplicationState->NewInputs;
    game_inputs* OldInputs = &ApplicationState->OldInputs;

    game_input_controller* OldKeyboardController = &OldInputs->Controllers[GAMEINPUT_KEYBOARD_INDEX];
    game_input_controller* NewKeyboardController = &NewInputs->Controllers[GAMEINPUT_KEYBOARD_INDEX];
    game_input_controller  ZeroController = {};

    for (unsigned ButtonIndex = 0;
        ButtonIndex < ARRAY_SIZE(NewKeyboardController->Buttons);
        ++ButtonIndex)
    {
    }

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* AppState, SDL_AppResult result)
{
    sdl_application_state* ApplicationState = (sdl_application_state*) AppState;

    delete ApplicationState;
}
