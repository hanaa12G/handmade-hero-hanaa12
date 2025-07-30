#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "sdl_handmade.h"

#include <cassert>
#include <cstring>

#ifndef HANDMDE_GLOBAL_DEFINE
#define HANDMDE_GLOBAL_DEFINE

#define global_variable static
#define local_persist static
#define internal_func static

#endif

#define ARRAY_SIZE(x) (sizeof((x)) / sizeof((x)[0]))

#define MAX_FRAME_KEYBOARD_EVENT 20

global_variable SDL_Window* Window;
global_variable SDL_Renderer* Renderer;


internal_func
void SDLAppReadKeyboardButtonState(game_button_state* NewState,
    int ButtonBit)
{
    SDL_Log("Current state is %d, set to %d", NewState->EndedDown, (int) ButtonBit);
    assert(NewState->EndedDown != (bool) ButtonBit);
    NewState->EndedDown = (bool) ButtonBit;
    NewState->HalfTransitionCount += 1;
}
internal_func
void SDLAppProcessPendingMessages(sdl_application_state* ApplicationState,
    game_input_controller* Controller)
{
    for (int EventIndex = 0;
        EventIndex < ApplicationState->KeyboardEventsCount; 
        ++EventIndex) {

        SDL_KeyboardEvent* Event = 
            &ApplicationState->KeyboardEvents[EventIndex];

        bool IsKeyDown  = Event->down;
        bool IsKeyRepeat = Event->repeat;
        SDL_Log("Event type is: %d, key=%d, down=%d, repeat=%d",
            Event->type, Event->scancode, IsKeyDown, IsKeyRepeat);

        if (!IsKeyRepeat) {
            SDL_Log("Set key state");
            switch (Event->scancode) {
                case SDL_SCANCODE_UP:
                    {
                        SDLAppReadKeyboardButtonState(&Controller->ActionUp,
                                IsKeyDown);
                    } break;
                case SDL_SCANCODE_DOWN:
                    {
                        SDLAppReadKeyboardButtonState(&Controller->ActionDown,
                                IsKeyDown);
                    } break;
                case SDL_SCANCODE_LEFT:
                    {
                        SDLAppReadKeyboardButtonState(&Controller->ActionLeft,
                                IsKeyDown);
                    } break;
                case SDL_SCANCODE_RIGHT:
                    {
                        SDLAppReadKeyboardButtonState(&Controller->ActionRight,
                                IsKeyDown);
                    } break;
                case SDL_SCANCODE_W:
                    {
                        SDLAppReadKeyboardButtonState(&Controller->MoveUp,
                                IsKeyDown);
                    } break;
                case SDL_SCANCODE_A:
                    {
                        SDLAppReadKeyboardButtonState(&Controller->MoveLeft,
                                IsKeyDown);
                    } break;
                case SDL_SCANCODE_S:
                    {
                        SDLAppReadKeyboardButtonState(&Controller->MoveDown,
                                IsKeyDown);
                    } break;
                case SDL_SCANCODE_D:
                    {
                        SDLAppReadKeyboardButtonState(&Controller->MoveRight,
                                IsKeyDown);
                    } break;
                case SDL_SCANCODE_Q:
                    {
                        ApplicationState->AppIsRunning = false;
                    } break;
#if HANDMADE_INTERNAL
                case SDL_SCANCODE_P:
                    {
                        if (IsKeyDown) {
                            ApplicationState->GlobalPause =
                                !ApplicationState->GlobalPause;
                        }
                    } break;
#endif
                case SDL_SCANCODE_F4:
                    {
                        bool IsAltDown = Event->mod & SDL_KMOD_LALT;
                        if (IsAltDown) {
                            ApplicationState->AppIsRunning = false;
                        }
                    } break;

                default:
                    {
                    } break;
            }
        } break;
    }
}

SDL_AppResult SDL_AppInit(void** AppState, int argc, char* argv[])
{
    sdl_application_state* ApplicationState =  new sdl_application_state();
    ApplicationState->PerformanceCounterFreq =  SDL_GetPerformanceFrequency();

    ApplicationState->KeyboardEvents = new SDL_KeyboardEvent[MAX_FRAME_KEYBOARD_EVENT];

    *AppState = ApplicationState;

    SDL_SetAppMetadata("handmade-hero-hanaa12", "0.1", NULL);
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    } if (!SDL_CreateWindowAndRenderer("Handmade Hero", 640, 480, 0, &Window, &Renderer)) {
        SDL_Log("Coundn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* AppState, SDL_Event* Event)
{
    sdl_application_state* ApplicationState = (sdl_application_state*) AppState;


    switch (Event->type) {
        case SDL_EVENT_QUIT:
        {
            return SDL_APP_SUCCESS;
        } break;
        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP:
        {
            assert(ApplicationState->KeyboardEventsCount <= MAX_FRAME_KEYBOARD_EVENT);

            SDL_KeyboardEvent* DstKeyboardEvent = 
                ApplicationState->KeyboardEvents + 
                ApplicationState->KeyboardEventsCount;
            SDL_KeyboardEvent* SrcKeyboardEvent = &Event->key;


            std::memcpy(DstKeyboardEvent, SrcKeyboardEvent,
                sizeof(SDL_KeyboardEvent));

            ApplicationState->KeyboardEventsCount += 1;

        } break;
        default:
        {
        } break;
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* AppState)
{
    sdl_application_state* ApplicationState = (sdl_application_state*) AppState;
    if (!ApplicationState->AppIsRunning) {
        SDL_Log("App finishes running");
        return SDL_APP_SUCCESS;
    }

    game_inputs* NewInputs = &ApplicationState->NewInputs;
    game_inputs* OldInputs = &ApplicationState->OldInputs;

    game_input_controller* OldKeyboardController = 
        &OldInputs->Controllers[GAMEINPUT_KEYBOARD_INDEX];
    game_input_controller* NewKeyboardController = 
        &NewInputs->Controllers[GAMEINPUT_KEYBOARD_INDEX];
    game_input_controller  ZeroController = {};

    *NewKeyboardController = ZeroController;

    for (unsigned ButtonIndex = 0;
        ButtonIndex < ARRAY_SIZE(NewKeyboardController->Buttons);
        ++ButtonIndex)
    {
        NewKeyboardController->Buttons[ButtonIndex].EndedDown = 
            OldKeyboardController->Buttons[ButtonIndex].EndedDown;
    }
    SDLAppProcessPendingMessages(ApplicationState, NewKeyboardController);

    *OldInputs = *NewInputs;
    *NewInputs = {};

    ApplicationState->KeyboardEventsCount = 0;

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* AppState, SDL_AppResult result)
{
    sdl_application_state* ApplicationState = (sdl_application_state*) AppState;
    
    delete ApplicationState->KeyboardEvents;
    delete ApplicationState;
}
