#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "sdl_handmade.h"
#include "handmade.cpp"
#include "circular_buffer.cpp"

#include <cassert>
#include <cstring>

#ifndef HANDMDE_GLOBAL_DEFINE
#define HANDMDE_GLOBAL_DEFINE

#define global_variable static
#define local_persist static
#define internal_func static

#endif

#define ARRAY_SIZE(x) (sizeof((x)) / sizeof((x)[0]))

#define KILOBYTES(n) 1024l * (n)
#define MEGABYTES(n) 1024l * KILOBYTES(n)
#define GIGABYTES(n) 1024l * MEGABYTES(n)

#define MAX_FRAME_KEYBOARD_EVENT 60
#define AUDIO_SAMPLES_PER_SECOND 48000

global_variable SDL_Window* Window;
global_variable SDL_Renderer* Renderer;
global_variable uint64_t PerformanceCounterFreq;
global_variable int const ExpectedFPS = 40;
global_variable double ExpectedFrameTime = 1.0 / ExpectedFPS;

internal_func
void SDLAppHandleAudioGetCallback(void* UserData, SDL_AudioStream* Stream,
    int AdditionalAmount, int TotalAmount)
{
    sdl_sound_output* SoundOutput = (sdl_sound_output*) UserData;

    SoundOutput->RunningSampleIndex += AdditionalAmount / 4;

    /* int RequestedSamples = AdditionalAmount / (sizeof(int16_t) * 2); */

    /* circular_buffer* SoundBuffer = &SoundOutput->Buffer; */

    /* // TODO (hanasou): Can we make this smaller (192KB currently) */
    /* char OutBuffer[AUDIO_SAMPLES_PER_SECOND * sizeof(int16_t) * 2] = {}; */

    /* int BytesCanRead = CircularBufferReadableSize(SoundBuffer); */
    /* // TODO (hanasou): Bit manipulation way */
    /* while (BytesCanRead > 0 && BytesCanRead % 4 != 0) { */
    /*     BytesCanRead -= 1; */
    /* } */
    /* int ReadBytes = std::min(AdditionalAmount, BytesCanRead); */
    /* SDL_Log("Read Byte %d", ReadBytes); */
    /* ReadBytes += 400; */
    /* SoundOutput->RunningSampleIndex += ReadBytes / 4; */

    /* SDL_Log("Get Callback: additional=%d total=%d, can_read=%d, out: %d", AdditionalAmount / 4, TotalAmount / 4, BytesCanRead/ 4, ReadBytes / 4); */

    /* CircularBufferRead(SoundBuffer, OutBuffer, ReadBytes); */

    /* SDL_PutAudioStreamData(Stream, OutBuffer, ReadBytes); */
    /* SDL_FlushAudioStream(Stream); */
}

internal_func
void SDLAppHandleAudioPutCallback(void* UserData, SDL_AudioStream* Stream,
    int AdditionalAmount, int TotalAmount)
{
    // SDL_Log("Put Callback: additional=%d total=%d", AdditionalAmount, TotalAmount);
}

internal_func
void SDLAppHandleAudioCallback(void* UserData, SDL_AudioStream* Stream,
    int AdditionalAmount, int TotalAmount)
{
    sdl_application_state* ApplicationState = (sdl_application_state*) UserData;

    AdditionalAmount /= 2 * sizeof(int16_t);
    TotalAmount /= 2 * sizeof(int16_t);


    ApplicationState->AudioAdditional += AdditionalAmount;
    ApplicationState->AudioTotal += TotalAmount;

    int QueuedBytes = SDL_GetAudioStreamQueued(Stream);
    assert(QueuedBytes % (sizeof(int16_t) * 2) == 0);

    int QueuedSamples = QueuedBytes / (sizeof(int16_t) * 2);

    int AvailableBytes = SDL_GetAudioStreamAvailable(Stream);
    assert(AvailableBytes % (sizeof(int16_t) * 2) == 0);

    int AvailableSamples = AvailableBytes / (sizeof(int16_t) * 2);
    SDL_Log("SDLAppHandleAudioCallback: %d - %d, queued_samples: %d, avai_samples: %d", AdditionalAmount, TotalAmount, QueuedSamples, AvailableSamples);
}

internal_func
float SDLAppGetSecondsElapsed(uint64_t From, uint64_t To) {
    return (To - From) / (double) PerformanceCounterFreq;
}

internal_func
void SDLAppReadKeyboardButtonState(game_button_state* NewState,
    int ButtonBit)
{
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
        bool IsKeyStateChanged = !IsKeyRepeat;

        /* SDL_Log("Set key state (%d/%d) %d, Down=%d, Repeat=%d", EventIndex, ApplicationState->KeyboardEventsCount, Event->scancode, IsKeyDown, IsKeyRepeat); */
        if (IsKeyStateChanged) {
            /* SDL_Log("Set key state %d", Event->scancode); */
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
        }
    }
}

internal_func
void SDLAppResizeBitmap(sdl_offscreen_buffer* Buffer, int Width, int Height)
{
    assert(Width >= 0 && Height >=0 && Buffer);

    if (Buffer->Surface) {
        SDL_DestroySurface(Buffer->Surface);
        Buffer->Surface = NULL;
    }

    Buffer->Surface = SDL_CreateSurface(Width, Height, SDL_PIXELFORMAT_BGRA32);
}

internal_func
void SDLAppUpdateWindow(sdl_offscreen_buffer* Buffer,
    SDL_Surface* ScreenSurface)
{
    SDL_BlitSurfaceScaled(Buffer->Surface, NULL,
        ScreenSurface, NULL,
        SDL_SCALEMODE_LINEAR);
    SDL_UpdateWindowSurface(Window);
}

SDL_AppResult SDL_AppInit(void** AppState, int argc, char* argv[])
{
    sdl_application_state* ApplicationState =  new sdl_application_state();

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
    SDL_SetWindowResizable(Window, true);

    PerformanceCounterFreq =  SDL_GetPerformanceFrequency();
    
    game_memory* GameMemory = &ApplicationState->GameMemory;
    GameMemory->PermanentStorageSize = MEGABYTES(32);
    GameMemory->TransientStorageSize = MEGABYTES(1);
    uint64_t TotalGameMemoryBytes = GameMemory->PermanentStorageSize + 
        GameMemory->TransientStorageSize;
    
    void* GameMemoryBuffer = SDL_malloc(TotalGameMemoryBytes);
    GameMemory->PermanentStorage = GameMemoryBuffer;
    GameMemory->TransientStorage = (char*) GameMemoryBuffer
        + GameMemory->PermanentStorageSize;
    
    
    if (!SDL_Init(SDL_INIT_AUDIO)) {
        SDL_Log("Couldn't initialize SDL Audio: %s", SDL_GetError());
    }
    else {

        SDL_AudioSpec AudioSpec = {
            .format = SDL_AUDIO_S16,
            .channels = 2,
            .freq = AUDIO_SAMPLES_PER_SECOND
        };
        SDL_AudioStream* AudioStream = SDL_OpenAudioDeviceStream(
            SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
            &AudioSpec,
            NULL,
            NULL
        );

        ApplicationState->AudioStream = AudioStream;
        SDL_ResumeAudioStreamDevice(ApplicationState->AudioStream);

        /* int const AudioBufferSize = 2 * sizeof(int16_t) * AUDIO_SAMPLES_PER_SECOND; */
        /* void* AudioBuffer = SDL_malloc(AudioBufferSize); */

        /* ApplicationState->SoundOutput.Memory = AudioBuffer; */
        /* CircularBufferInit(&ApplicationState->SoundOutput.Buffer, */
        /*     ApplicationState->SoundOutput.Memory, */
        /*     AudioBufferSize); */

    }



    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* AppState, SDL_Event* Event)
{
    sdl_application_state* ApplicationState = (sdl_application_state*) AppState;

    char Buffer[1024] = {};
    SDL_GetEventDescription(Event, Buffer, 1024);

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
            /* SDL_Log("ApplicationState->KeyboardEventsCount: %d", ApplicationState->KeyboardEventsCount); */

        } break;
        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
        {
            SDL_WindowEvent* WEvent = &Event->window;
            SDLAppResizeBitmap(&ApplicationState->Buffer, WEvent->data1,
                WEvent->data2);
        } break;
        case SDL_EVENT_WINDOW_EXPOSED:
        {
            SDL_Surface* ScreenSurface = SDL_GetWindowSurface(Window);
            SDLAppUpdateWindow(&ApplicationState->Buffer, ScreenSurface);
        } break;
        default:
        {
        } break;
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* AppState)
{
    SDL_Log("Frame Begin");
    uint64_t FrameStartCounter = SDL_GetPerformanceCounter();
    uint64_t CurrentTimerCounter = FrameStartCounter;
    double   Elapsed = 0.0;

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

    SDL_Surface* ScreenSurface = SDL_GetWindowSurface(Window);

    if (!ApplicationState->GlobalPause) {

        game_offscreen_buffer GameDrawingBuffer = {};
        GameDrawingBuffer.Data   = ApplicationState->Buffer.Surface->pixels;
        GameDrawingBuffer.Width  = ApplicationState->Buffer.Surface->w;
        GameDrawingBuffer.Height = ApplicationState->Buffer.Surface->h;
        GameDrawingBuffer.Pitch  = ApplicationState->Buffer.Surface->pitch;

        GameUpdateAndRender(&ApplicationState->GameMemory, &GameDrawingBuffer,
                NewInputs);

        SDL_LockAudioStream(ApplicationState->AudioStream);

        sdl_sound_output* SoundOutput = &ApplicationState->SoundOutput;

        double SamplesPerFrame = (double) AUDIO_SAMPLES_PER_SECOND * ExpectedFrameTime;
        CurrentTimerCounter = SDL_GetPerformanceCounter();
        Elapsed = SDLAppGetSecondsElapsed(FrameStartCounter, CurrentTimerCounter);
        double ExpectedSecondsUntilFlip = ExpectedFrameTime - Elapsed;
        int ExpectedSamplesUntilFlip = (int) ((ExpectedSecondsUntilFlip / ExpectedFrameTime)
            * SamplesPerFrame);
        
        int SamplesUntilNextFrame = ExpectedSamplesUntilFlip + SamplesPerFrame;
        int QueuedSamples = SDL_GetAudioStreamQueued(ApplicationState->AudioStream);
        QueuedSamples /= 4;
        int SamplesToWrite = SamplesPerFrame;

        if (SamplesUntilNextFrame > QueuedSamples) {
            SamplesToWrite = SamplesUntilNextFrame - QueuedSamples;
        }

        SDL_Log("Expect Frames: %d, Queued: %d, Written: %d", ExpectedSamplesUntilFlip, QueuedSamples, SamplesToWrite);

        const int Channels = 2;
        const int SamplesPerSecond = AUDIO_SAMPLES_PER_SECOND;
        int16_t Samples[SamplesPerSecond * Channels] = {};
        game_sound_output_buffer GameSoundBuffer = {};

        GameSoundBuffer.SamplesPerSecond = SamplesPerSecond;
        GameSoundBuffer.SampleCountToOutput = SamplesToWrite;
        GameSoundBuffer.Samples = Samples;
        
        GameGetSoundSample(&ApplicationState->GameMemory, &GameSoundBuffer);


        int BytesToWrite = SamplesToWrite * Channels * sizeof(int16_t);
        SDL_PutAudioStreamData(ApplicationState->AudioStream, Samples, BytesToWrite);
        SDL_FlushAudioStream(ApplicationState->AudioStream);

        SDL_UnlockAudioStream(ApplicationState->AudioStream);

        CurrentTimerCounter = SDL_GetPerformanceCounter();
        Elapsed = SDLAppGetSecondsElapsed(FrameStartCounter, CurrentTimerCounter);

        uint64_t ElapsedNS = (uint64_t) (Elapsed * 1000'000'000ul);
        uint64_t ExpectedFrameElapsedNS = (uint64_t) (ExpectedFrameTime * 1'000'000'000ul);

        if (ExpectedFrameElapsedNS > ElapsedNS) {
            uint64_t NSToSleep = ExpectedFrameElapsedNS - ElapsedNS;

            SDL_DelayPrecise(NSToSleep);
        }
        else {
            // TODO (hanasou): We missed a frame, should put a log here
        }

        SDLAppUpdateWindow(&ApplicationState->Buffer, ScreenSurface);


        *OldInputs = *NewInputs;
        *NewInputs = {};

    }

    ApplicationState->KeyboardEventsCount = 0;

    SDL_Log("Frame end");
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* AppState, SDL_AppResult result)
{
    sdl_application_state* ApplicationState = (sdl_application_state*) AppState;

    SDL_free(ApplicationState->SoundOutput.Memory);
    SDL_DestroyAudioStream(ApplicationState->AudioStream);
    
    SDL_DestroySurface(ApplicationState->Buffer.Surface);
    delete ApplicationState->KeyboardEvents;
    delete ApplicationState;
}
