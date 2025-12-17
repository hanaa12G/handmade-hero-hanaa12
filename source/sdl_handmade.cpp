#include <SDL3/SDL_iostream.h>
#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "sdl_handmade.h"
#include "handmade.hpp"
#include "circular_buffer.cpp"

#include <cassert>
#include <cstring>

#include <iostream>

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
#define MAX_PATH 1024

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

global_variable SDL_Window* Window;
global_variable SDL_Renderer* Renderer;
global_variable uint64_t PerformanceCounterFreq;
global_variable int const ExpectedFPS = 60;
global_variable double ExpectedFrameTime = 1.0 / ExpectedFPS;
char *SourceDllPath = nullptr;
char *TmpDllPath = nullptr;

internal_func void
StringConcat(int SourceACount, char const* SourceA,
                  int SourceBCount, char const* SourceB,
                  int DestCount,    char* Dest);
internal_func int
StringLength(char const* String);

internal_func int
StringCopy(char* Dest, char const* Source, int BytesToCopy);


void* GameBaseAddress = NULL;
u64   GameMemorySize  = 0;

internal_func
void* GetGameActualAddress(u64 Offset)
{
    assert(Offset < GameMemorySize);
    return (u8*) GameBaseAddress  + Offset;
}

internal_func
u64 GetGameMemoryHandle(void* Addr)
{
    assert((u8*) Addr - (u8*) GameBaseAddress >= 0
        && (u64) ((u8*) Addr - (u8*) GameBaseAddress) < GameMemorySize);

    return (u64) ((u8*) Addr - (u8*) GameBaseAddress);
}


internal_func
void StringConcat(int SourceACount, char const* SourceA,
               int SourceBCount, char const* SourceB,
               int DestCount,    char* Dest)
{
    int Bytes = StringCopy(Dest, SourceA, SourceACount);
    StringCopy(Dest + Bytes, SourceB, SourceBCount);
}

internal_func
int StringLength(char const* String)
{
    int Length = 0;
    for (; *String != '\0'; ++String) Length += 1;

    return Length;
}

internal_func
int StringCopy(char* Dest, char const* Source, int BytesToCopy)
{
    int Byte = 0;
    for (; *Source != '\0' && Byte < BytesToCopy; ) {
        *Dest = *Source;
        Dest += 1;
        Source += 1;
        Byte += 1;
    }

    *Dest = '\0';

    return Byte;
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

internal_func void
SDLAppRecordingInput(sdl_record_state* State, game_inputs const* Input)
{
    int64_t Bytes = SDL_WriteIO(State->RecordingStream, Input, sizeof(game_inputs));

    (void) Bytes;
}

internal_func void
SDLAppPlaybackInput(sdl_record_state* State, game_inputs* Input)
{
    int64_t Bytes = SDL_ReadIO(State->PlaybackStream, Input, sizeof(game_inputs));
    if (Bytes == 0) {
        if (SDL_IO_STATUS_EOF == SDL_GetIOStatus(State->PlaybackStream)) {
            SDL_SeekIO(State->PlaybackStream, 0, SDL_IO_SEEK_SET);
            (void) SDL_ReadIO(State->PlaybackStream, State->MemoryBlock, State->MemorySize);
            (void) SDL_ReadIO(State->PlaybackStream, Input, sizeof(game_inputs));
        }
    }
}

internal_func void
SDLAppBeginRecordingInput(sdl_record_state* State, int RecordingIndex) 
{
    State->RecordingStream = SDL_IOFromFile("handmade.input", "wb");
    State->RecordingIndex = RecordingIndex;

    /* SDL_WriteIO(State->RecordingStream, &State->MemoryStartAddress, sizeof(State->MemoryStartAddress)); */
    SDL_WriteIO(State->RecordingStream, State->MemoryBlock, State->MemorySize);
}

internal_func void
SDLAppEndRecordingInput(sdl_record_state* State)
{
    (void) SDL_CloseIO(State->RecordingStream);
    State->RecordingIndex = 0;
    State->PlaybackStream = NULL;
}

internal_func void
SDLAppBeginPlaybackInput(sdl_record_state* State, int PlaybackIndex)
{
    State->PlaybackStream = SDL_IOFromFile("handmade.input", "rb");
    State->PlaybackIndex = PlaybackIndex;

    SDL_ReadIO(State->PlaybackStream, State->MemoryBlock, State->MemorySize);
}

internal_func void
SDLAppEndPlaybackInput(sdl_record_state* State)
{
    (void) SDL_CloseIO(State->PlaybackStream);
    State->PlaybackIndex = 0;
    State->PlaybackStream = NULL;
}


internal_func
void SDLAppProcessPendingMessages(sdl_application_state* ApplicationState,
    game_input_controller* Controller)
{
    sdl_record_state* RecordingState = &ApplicationState->RecordingState;
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
                case SDL_SCANCODE_L:
                    {
                        if (IsKeyDown) {
                            if (RecordingState->RecordingIndex == 0) {
                                if (RecordingState->PlaybackStream) {
                                    SDLAppEndPlaybackInput(RecordingState);
                                }
                                SDLAppBeginRecordingInput(RecordingState, 1);
                            }
                            else {
                                SDLAppEndRecordingInput(RecordingState);
                                SDLAppBeginPlaybackInput(RecordingState, 1);
                            }
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

    Buffer->Width = Width;
    Buffer->Height = Height;
    Buffer->BytesPerPixel = 4;
    Buffer->Pitch = Buffer->BytesPerPixel * Buffer->Width;
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

internal_func
SDL_Time SDLGetLastWriteTime(char const* filename)
{
    SDL_PathInfo info = {};
    if (!SDL_GetPathInfo(filename, &info)) {
        return 0;
    }

    return info.modify_time;
}


internal_func
sdl_game_code SDLAppLoadGameCode(char const* SourceDllPath, char const* TmpDllPath)
{
    sdl_game_code Result = {};


    if (!SDL_CopyFile(SourceDllPath, TmpDllPath)) {
        SDL_Log("Error copy file: %s", SDL_GetError());
        return {};
    }

    Result.DllLastWriteTime = SDLGetLastWriteTime(SourceDllPath);

    SDL_SharedObject* GameCode = SDL_LoadObject(TmpDllPath);
    if (!GameCode) {
        SDL_Log("Cannot load game code %s: %s", TmpDllPath, SDL_GetError());
        return Result;
    }

    Result.Object = GameCode;

    SDL_FunctionPointer GameUpdateAndRenderFunc = SDL_LoadFunction(GameCode, "GameUpdateAndRender");
    SDL_FunctionPointer GameGetSoundSampleFunc = SDL_LoadFunction(GameCode, "GameGetSoundSample");

    if (GameUpdateAndRenderFunc) {
        Result.GameUpdateAndRender = (game_update_and_render*) GameUpdateAndRenderFunc;
    }
    if (GameGetSoundSampleFunc) {
        Result.GameGetSoundSample = (game_get_sound_sample*) GameGetSoundSampleFunc;
    }

    Result.IsValid = GameUpdateAndRenderFunc && GameGetSoundSampleFunc;

    return Result;
}

internal_func
void SDLAppUnloadGameCode(sdl_game_code* GameCode)
{
    GameCode->IsValid = false;
    GameCode->GameUpdateAndRender = &GameUpdateAndRenderStub;
    GameCode->GameGetSoundSample = &GameGetSoundSampleStub;

    SDL_UnloadObject(GameCode->Object);
    GameCode->Object = NULL;
}

DEBUG_PLATFORM_READ_ENTIRE_FILE(DEBUGPlatformReadEntireFile)
{
    SDL_Storage* User = SDL_OpenUserStorage("hanasou", "handmade-hero-hanaa12", 0);
    if (User == NULL) {
        return false;
    }
    while (!SDL_StorageReady(User)) {
        SDL_Delay(1);
    }

    uint64_t FileSize = 0;

    if (!SDL_GetStorageFileSize(User, FileName, &FileSize)) {
        SDL_CloseStorage(User);
        SDL_Log("Cannot get file size %s", SDL_GetError());
        return false;
    }

    void* Buffer = SDL_malloc(FileSize);
    bool ReadOK = false;
    if (Buffer) {
        if (SDL_ReadStorageFile(User, FileName, Buffer, FileSize)) {
            Result->Size = FileSize;   
            Result->Memory = Buffer;
            ReadOK = true;
        } else {
            SDL_free(Buffer);
        }
    }

    SDL_CloseStorage(User);

    return ReadOK;
}

DEBUG_PLATFORM_WRITE_ENTINE_FILE(DEBUGPlatformWriteEntireFile)
{
    SDL_Storage* User = SDL_OpenUserStorage("hanasou", "handmade-hero-hanaa12", 0);
    if (User == NULL) {
        return false;
    }
    while (!SDL_StorageReady(User)) {
        SDL_Delay(1);
    }

    bool WriteOK = true;
    if (!SDL_WriteStorageFile(User, FileName, Buffer, Size)) {
        SDL_Log("Cannot write file %s", SDL_GetError());
        WriteOK = false;
    }

    SDL_CloseStorage(User);
    return WriteOK;
}

internal_func void 
DEBUGPlatformFreeFileMemory(debug_file_result* File)
{
  SDL_free(File->Memory);
}

SDL_AppResult SDL_AppInit(void** AppState, int, char* [])
{
    char const* BasePath = SDL_GetBasePath();
    char const SourceGameCodeDllFileName[] = "libhandmade.so";
    char const TmpGameCodeDllFilename[]    = "libhandmade_tmp.so";
    char       SourceGameCodeDllFullPath[MAX_PATH];
    char       TmpGameCodeDllFullPath[MAX_PATH];

    StringConcat(StringLength(BasePath) + 1, BasePath,
                 sizeof(SourceGameCodeDllFileName), SourceGameCodeDllFileName,
                 sizeof(SourceGameCodeDllFullPath), SourceGameCodeDllFullPath);
    StringConcat(StringLength(BasePath) + 1, BasePath, 
                 sizeof(TmpGameCodeDllFilename), TmpGameCodeDllFilename,
                 sizeof(TmpGameCodeDllFullPath), TmpGameCodeDllFullPath);

    int SourceGameCodeDllFullPathLength = StringLength(SourceGameCodeDllFullPath);
    int TmpGameCodeDllFullPathLength    = StringLength(TmpGameCodeDllFullPath);

    sdl_application_state* ApplicationState =  new sdl_application_state();


    ApplicationState->KeyboardEvents = new SDL_KeyboardEvent[MAX_FRAME_KEYBOARD_EVENT];

    *AppState = ApplicationState;

    SDL_SetAppMetadata("handmade-hero-hanaa12", "0.1", NULL);
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    } if (!SDL_CreateWindowAndRenderer("Handmade Hero", 640, 480, SDL_WINDOW_ALWAYS_ON_TOP, &Window, &Renderer)) {
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
    GameMemory->PermanentStorage = 0;
    GameMemory->TransientStorage = GameMemory->PermanentStorageSize;

    GameMemory->GetPtr = GetGameActualAddress;
    GameMemory->GetHandle = GetGameMemoryHandle;

    GameBaseAddress = GameMemoryBuffer;
    GameMemorySize  = TotalGameMemoryBytes;

    ApplicationState->RecordingState.MemorySize         = TotalGameMemoryBytes;
    ApplicationState->RecordingState.MemoryBlock        = GameMemoryBuffer;
#ifdef HANDMADE_INTERNAL
    GameMemory->DEBUGPlatformReadEntireFile = &DEBUGPlatformReadEntireFile;
    GameMemory->DEBUGPlatformWriteEntireFile = &DEBUGPlatformWriteEntireFile;
    GameMemory->DEBUGPlatformFreeFileMemory = &DEBUGPlatformFreeFileMemory;
#endif
    
    
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
    }

    /* ApplicationState->UserStorage = SDL_OpenUserStorage("hanasou", "handmade-hero-hanaa12", 0); */
    /* while (!SDL_StorageReady(ApplicationState->UserStorage)) { */
    /*     SDL_Delay(1); */
    /* } */

    /* assert(ApplicationState->UserStorage); */


    ApplicationState->SourceGameCodeDllFullPath = 
        (char*) SDL_malloc(SourceGameCodeDllFullPathLength + 1);
    ApplicationState->TmpGameCodeDllFullPath    = 
        (char*) SDL_malloc(TmpGameCodeDllFullPathLength + 1);

    StringCopy(ApplicationState->SourceGameCodeDllFullPath,
               SourceGameCodeDllFullPath,
               SourceGameCodeDllFullPathLength);

    StringCopy(ApplicationState->TmpGameCodeDllFullPath,
               TmpGameCodeDllFullPath,
               TmpGameCodeDllFullPathLength);

    ApplicationState->GameCode = SDLAppLoadGameCode(
        ApplicationState->SourceGameCodeDllFullPath,
        ApplicationState->TmpGameCodeDllFullPath
    );


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
    uint64_t FrameStartCounter = SDL_GetPerformanceCounter();
    uint64_t CurrentTimerCounter = FrameStartCounter;
    double   Elapsed = 0.0;

    sdl_application_state* ApplicationState = (sdl_application_state*) AppState;
    if (!ApplicationState->AppIsRunning) {
        SDL_Log("App finishes running");
        return SDL_APP_SUCCESS;
    }

    SDL_Time DllWriteTime = SDLGetLastWriteTime(SourceDllPath);
    if (DllWriteTime != ApplicationState->GameCode.DllLastWriteTime) {
        SDLAppUnloadGameCode(&ApplicationState->GameCode);
        ApplicationState->GameCode = SDLAppLoadGameCode(
            ApplicationState->SourceGameCodeDllFullPath,
            ApplicationState->TmpGameCodeDllFullPath
        );
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
    if (ApplicationState->RecordingState.RecordingIndex) {
        SDLAppRecordingInput(&ApplicationState->RecordingState, NewInputs);
    }
    else if (ApplicationState->RecordingState.PlaybackIndex) {
        SDLAppPlaybackInput(&ApplicationState->RecordingState, NewInputs);
    }

    SDL_Surface* ScreenSurface = SDL_GetWindowSurface(Window);

    if (!ApplicationState->GlobalPause) {

        game_offscreen_buffer GameDrawingBuffer = {};
        GameDrawingBuffer.Data   = ApplicationState->Buffer.Surface->pixels;
        GameDrawingBuffer.Width  = ApplicationState->Buffer.Surface->w;
        GameDrawingBuffer.Height = ApplicationState->Buffer.Surface->h;
        GameDrawingBuffer.Pitch  = ApplicationState->Buffer.Surface->pitch;
        GameDrawingBuffer.BytesPerPixel = ApplicationState->Buffer.BytesPerPixel;

        ApplicationState->GameCode.GameUpdateAndRender(&ApplicationState->GameMemory, &GameDrawingBuffer,
                NewInputs);

        SDL_LockAudioStream(ApplicationState->AudioStream);

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

        /* SDL_Log("Expect Frames: %d, Queued: %d, Written: %d", ExpectedSamplesUntilFlip, QueuedSamples, SamplesToWrite); */

        const int Channels = 2;
        const int SamplesPerSecond = AUDIO_SAMPLES_PER_SECOND;
        int16_t Samples[SamplesPerSecond * Channels] = {};
        game_sound_output_buffer GameSoundBuffer = {};

        GameSoundBuffer.SamplesPerSecond = SamplesPerSecond;
        GameSoundBuffer.SampleCountToOutput = SamplesToWrite;
        GameSoundBuffer.Samples = Samples;
        
        ApplicationState->GameCode.GameGetSoundSample(&ApplicationState->GameMemory, &GameSoundBuffer);


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

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* AppState, SDL_AppResult)
{
    sdl_application_state* ApplicationState = (sdl_application_state*) AppState;

    SDL_free(ApplicationState->SourceGameCodeDllFullPath);
    SDL_free(ApplicationState->TmpGameCodeDllFullPath);

    /* SDL_CloseStorage(ApplicaitionState->UserStorage); */

    SDL_free(SourceDllPath);
    SDL_free(TmpDllPath);
    SDL_UnloadObject(ApplicationState->GameCode.Object);

    SDL_DestroyAudioStream(ApplicationState->AudioStream);
    
    SDL_DestroySurface(ApplicationState->Buffer.Surface);
    delete ApplicationState->KeyboardEvents;
    delete ApplicationState;
}
