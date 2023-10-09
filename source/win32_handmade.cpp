#include <windows.h>
#include <cstdint>
#include <cassert>
#include <cmath>
#include <cstdio>

#include <xinput.h>
#include <dsound.h>


#include "handmade.cpp" 
#define PI 3.14159265359f

#ifndef HANDMDE_GLOBAL_DEFINE
#define HANDMDE_GLOBAL_DEFINE
#define global_variable static
#define local_persist static
#define internal_func static
#endif

// DWORD WINAPI XInputGetState (DWORD dwUserIndex, XINPUT_STATE* pState)
using x_input_get_state = DWORD WINAPI (DWORD, XINPUT_STATE*);
DWORD WINAPI XInputGetStateStub (DWORD, XINPUT_STATE*)
{
  return ERROR_DEVICE_NOT_CONNECTED;
}

// DWORD WINAPI name (DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
using x_input_set_state = DWORD WINAPI (DWORD, XINPUT_VIBRATION*);
DWORD WINAPI XInputSetStateStub(DWORD, XINPUT_VIBRATION*)
{
  return ERROR_DEVICE_NOT_CONNECTED;
}

#define XInputGetState XInputGetState_
#define XInputSetState XInputSetState_
global_variable x_input_get_state* XInputGetState = &XInputGetStateStub;
global_variable x_input_set_state* XInputSetState = &XInputSetStateStub;


//HRESULT WINAPI DirectSoundCreate8(_In_opt_ LPCGUID pcGuidDevice, _Outptr_ LPDIRECTSOUND8 *ppDS8, _Pre_null_ LPUNKNOWN pUnkOuter);
using direct_sound_create = HRESULT WINAPI (LPCGUID, LPDIRECTSOUND8 *, LPUNKNOWN);
#define DirectSoundCreate DirectSoundCreate_
global_variable direct_sound_create* DirectSoundCreate;


internal_func void
LoadXInputDll()
{
  local_persist const wchar_t DllName1_r [] = L"xinput1_4.dll";
  local_persist const wchar_t DllName2_r [] = L"xinput1_3.dll";

  HMODULE Handle = LoadLibraryW(DllName1_r);
  if (Handle == NULL)
  {
    Handle = LoadLibraryW(DllName2_r);
  }

  if (Handle != NULL)
  {
    XInputGetState = (x_input_get_state*) GetProcAddress(Handle, "XInputGetState");
    XInputSetState = (x_input_set_state*) GetProcAddress(Handle, "XInputSetState");
    assert(XInputGetState && XInputSetState);
  }
  else
  {
    OutputDebugStringW(L"Can't load Xinput\n");
  }
}


global_variable bool       Win32AppIsRunning;
global_variable int        BytesPerPixel = 4;
global_variable bool       ShouldDisplayDebugInfo = false;

struct win32_offscreen_buffer
{
  BITMAPINFO BitMapInfo;
  void*      Data;
  int        Width;
  int        Height;
  int        Pitch;
};

struct win32_window_size
{
  int Width;
  int Height;
};

struct win32_sound_output
{
  unsigned ToneHz;
  unsigned SamplesPerSecond;
  unsigned BytesPerSample;
  int16_t  Volume;
  unsigned SecondaryBufferSize;
  unsigned SampleRunningIndex;
  unsigned WavePeriod;

  float    Angle;


  float t;
};

struct win32_application_data
{
  win32_offscreen_buffer Buffer;
  LPDIRECTSOUNDBUFFER    SecondarySoundBuffer;

  win32_sound_output*    SoundOutput;
  game_input_controllers NewInputs;
  game_input_controllers OldInputs;
};


internal_func void
Win32PrintPerfInformation(long long EslapsedTime, int FPS, unsigned long long ProcessorCounterEslapsed)
{
  if (ShouldDisplayDebugInfo)
  {
    char TimeDebugStr[128];
    std::snprintf(TimeDebugStr, 128, "Frame: %lldms, FPS: %d, ProcessorCounter: %lluM\n", EslapsedTime, FPS, ProcessorCounterEslapsed / 1000000);
    OutputDebugStringA(TimeDebugStr);
  }
}


internal_func void
Win32ReadXInputButtonState(game_button_state* OldState, game_button_state* NewState, int ButtonBit)
{
  NewState->IsButtonEndedDown = ButtonBit;
  NewState->HalfTransitionCount = (NewState->IsButtonEndedDown != OldState->IsButtonEndedDown) ? 1 : 0;
}


internal_func void
RenderWeirdRectangle(win32_offscreen_buffer* Buffer, int XOffset, int YOffset)
{
  uint8_t* Rows = (uint8_t*) Buffer->Data;
  for (int Y = 0;
       Y < Buffer->Height;
       ++Y)
  {
    uint32_t* Pixel = (uint32_t*) Rows;
    for (int X = 0;
         X < Buffer->Width;
         ++X)
    {
      /*
      Memory: RR GG BB xx
      Register: xx RR GG BB
      */

      uint8_t Green = (uint8_t) (XOffset + X);
      uint8_t Blue  = (uint8_t) (YOffset + Y);

      *Pixel++ = (uint32_t)((Green << 8) | Blue);

    }

    Rows += BytesPerPixel * Buffer->Width;
  }
}


internal_func void
Win32ResizeDIBSection(win32_offscreen_buffer* Buffer, int X, int Y, int Width, int Height)
{
  assert(Width >= 0 && Height >= 0 && Buffer);
  if (Buffer->Data)
  {
    VirtualFree(Buffer->Data, 0, MEM_RELEASE);
    Buffer->Data = NULL;
  }

  int BitsPerPixel = 32;
  Buffer->Width = Width;
  Buffer->Height = Height;
  Buffer->Pitch = BytesPerPixel * Buffer->Width;

  Buffer->BitMapInfo.bmiHeader.biSize   = sizeof(Buffer->BitMapInfo.bmiHeader);
  Buffer->BitMapInfo.bmiHeader.biWidth  = Buffer->Width;
  Buffer->BitMapInfo.bmiHeader.biHeight = -Buffer->Height;
  Buffer->BitMapInfo.bmiHeader.biPlanes  = 1;
  Buffer->BitMapInfo.bmiHeader.biBitCount = BitsPerPixel;
  Buffer->BitMapInfo.bmiHeader.biCompression = BI_RGB;


  int TotalBytes = BytesPerPixel * Buffer->Width * Buffer->Height;
  Buffer->Data = (uint8_t*) VirtualAlloc(0, TotalBytes, MEM_COMMIT, PAGE_READWRITE);

  RenderWeirdRectangle(Buffer, 0, 0);
}

internal_func void
Win32UpdateWindow(win32_offscreen_buffer* Buffer,
                  HDC DeviceContext,
                  RECT* ClientRect,
                  int X, int Y, int Width, int Height)
{
  StretchDIBits(DeviceContext,
  /*
                X, Y, Width, Height,
                X, Y, Width, Height,
   */
                0, 0, ClientRect->right - ClientRect->left, ClientRect->bottom - ClientRect->top,
                0, 0, Buffer->Width, Buffer->Height,
                Buffer->Data,
                &Buffer->BitMapInfo,
                DIB_RGB_COLORS, SRCCOPY);
}

internal_func LPDIRECTSOUNDBUFFER
Win32InitDSound(HWND Window, unsigned int SamplesPerSec, unsigned int BufferSize)
{
  local_persist const wchar_t DllName_r[] = L"dsound.dll";
  HMODULE Handle = LoadLibraryW(DllName_r);
  
  if (Handle == NULL)
  {
    // TODO (hanasou): Diagnostic
    return NULL;
  }

  DirectSoundCreate = (direct_sound_create*) GetProcAddress(Handle, "DirectSoundCreate");
  assert(DirectSoundCreate);

  LPDIRECTSOUND8 SoundDevice;
  if (!SUCCEEDED(DirectSoundCreate(NULL, &SoundDevice, NULL)))
  {
    // TODO (hanasou): Diagnostic
    return NULL;
  }

  if (!SUCCEEDED(SoundDevice->SetCooperativeLevel(Window, DSSCL_PRIORITY)))
  {
    // TODO (hanasou): Diagnostic
    return NULL;
  }
  DSBUFFERDESC BufferDesc = {};
  BufferDesc.dwSize = sizeof(BufferDesc);
  BufferDesc.dwFlags = DSBCAPS_PRIMARYBUFFER;
  LPDIRECTSOUNDBUFFER PrimaryBuffer;
  HRESULT Hr = SoundDevice->CreateSoundBuffer(&BufferDesc, &PrimaryBuffer, NULL);
  if (!SUCCEEDED(Hr))
  {
    // TODO (hanaosou): Diagnostic
    return NULL;
  }

  // Sample format: [LEFT] or [RIGHT]
  // [LEFT] [RIGHT] [LEFT] [RIGHT] .... [LEFT] [RIGHT]
  // |--48k samples per second-----|

  int NumChannels = 2;
  int BitsPerSample = 16;

  WAVEFORMATEX WaveFormat = {};
  WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
  WaveFormat.nChannels = NumChannels;
  WaveFormat.nSamplesPerSec = SamplesPerSec;
  WaveFormat.wBitsPerSample = BitsPerSample;
  // NOTE (hanasou): We calculate 
  WaveFormat.nBlockAlign = (NumChannels * BitsPerSample) / 8;
  WaveFormat.nAvgBytesPerSec = WaveFormat.nBlockAlign * SamplesPerSec;
  // 
  WaveFormat.cbSize = 0;

  Hr = PrimaryBuffer->SetFormat(&WaveFormat);
  if (!SUCCEEDED(Hr))
  {
    // TODO (hanasou): Diagnostic
    return NULL;
  }

  ZeroMemory(&BufferDesc, sizeof(BufferDesc));
  BufferDesc.dwSize = sizeof(BufferDesc);
  BufferDesc.dwBufferBytes = BufferSize;
  BufferDesc.lpwfxFormat = &WaveFormat;

  LPDIRECTSOUNDBUFFER SecondaryBuffer;
  Hr = SoundDevice->CreateSoundBuffer(&BufferDesc, &SecondaryBuffer, NULL);
  if (!SUCCEEDED(Hr))
  {
    // TODO (hanasou): Diagnostic
    return NULL;
  }

  return SecondaryBuffer;
}

internal_func void
Win32FillSoundBuffer(LPDIRECTSOUNDBUFFER SoundBuffer,
  win32_sound_output* SoundOutput, unsigned ByteToLock, unsigned BytesToWrite,
  game_sound_output_buffer* GameSoundBuffer)
{
  LPVOID Region1;
  DWORD Region1Size;
  LPVOID Region2;
  DWORD Region2Size;
  if (SUCCEEDED(SoundBuffer->Lock(ByteToLock, BytesToWrite,
          &Region1, &Region1Size,
          &Region2, &Region2Size,
          0)))
  {
    int Region1SampleSize = Region1Size / SoundOutput->BytesPerSample;
    int16_t* SampleOut = (int16_t*) Region1;
    int16_t* SampleIn = GameSoundBuffer->Samples;
    for (int Region1Index = 0;
        Region1Index < Region1SampleSize;
        ++Region1Index)
    {
      int16_t SampleValue = *SampleIn++;

      *SampleOut++ = SampleValue;
      *SampleOut++ = SampleValue;
    }

    SampleOut = (int16_t*) Region2;
    int Region2SampleSize = Region2Size / SoundOutput->BytesPerSample;
    for (int Region2Index = 0;
        Region2Index < Region2SampleSize;
        ++Region2Index)
    {
      int16_t SampleValue = *SampleIn++;
      *SampleOut++ = SampleValue;
      *SampleOut++ = SampleValue;
    }
    if (SUCCEEDED(SoundBuffer->Unlock(Region1, Region1Size, Region2, Region2Size)))
    {
    }
    else
    {
      // TODO (hanasou): Diagnostic
    }

  }
  else
  {
    // TODO (hanasou): Diagnostic
  }
  // TODO (hanasou): Play Sound
}

internal_func void
Win32ClearSoundBuffer(LPDIRECTSOUNDBUFFER SoundBuffer, win32_sound_output* SoundOutput)
{
  LPVOID Region1;
  DWORD Region1Size;
  LPVOID Region2;
  DWORD Region2Size;

  unsigned ByteToLock = 0;
  unsigned BytesToWrite = SoundOutput->SecondaryBufferSize;

  if (SUCCEEDED(SoundBuffer->Lock(ByteToLock, BytesToWrite,
          &Region1, &Region1Size,
          &Region2, &Region2Size,
          0)))
  {
    int Region1SampleSize = Region1Size / SoundOutput->BytesPerSample;
    int16_t* SampleOut = (int16_t*) Region1;
    for (int Region1Index = 0;
        Region1Index < Region1SampleSize;
        ++Region1Index)
    {
      int16_t SampleValue = 0;

      *SampleOut++ = SampleValue;
      *SampleOut++ = SampleValue;
    }

    SampleOut = (int16_t*) Region2;
    int Region2SampleSize = Region2Size / SoundOutput->BytesPerSample;
    for (int Region2Index = 0;
        Region2Index < Region2SampleSize;
        ++Region2Index)
    {
      int16_t SampleValue = 0;
      *SampleOut++ = SampleValue;
      *SampleOut++ = SampleValue;
    }
    if (SUCCEEDED(SoundBuffer->Unlock(Region1, Region1Size, Region2, Region2Size)))
    {
    }
    else
    {
      // TODO (hanasou): Diagnostic
    }

  }
  else
  {
    // TODO (hanasou): Diagnostic
  }
  // TODO (hanasou): Play Sound
}

internal_func LRESULT
Win32WindowProc(HWND Window,
                UINT    Msg,
                WPARAM  WParam,
                LPARAM  LParam
)
{
  auto ApplicationData = (win32_application_data*) GetWindowLongPtrW(Window, GWLP_USERDATA);
  LRESULT Res = 0;
  switch (Msg)
  {
    case WM_PAINT:
    {
      PAINTSTRUCT PaintInfo;
      BeginPaint(Window, &PaintInfo);
      RECT ClientRect;
      GetClientRect(Window, &ClientRect);
      int X = ClientRect.left;
      int Y = ClientRect.top;
      int Width = ClientRect.right - ClientRect.left;
      int Height = ClientRect.bottom - ClientRect.top;
      Win32UpdateWindow(&ApplicationData->Buffer, PaintInfo.hdc, &ClientRect, X, Y, Width, Height);
      EndPaint(Window, &PaintInfo);

    } break;
    case WM_SIZE:
    {
      RECT ClientRect;
      GetClientRect(Window, &ClientRect);
      int X = ClientRect.left;
      int Y = ClientRect.top;
      int Width = ClientRect.right - ClientRect.left;
      int Height = ClientRect.bottom - ClientRect.top;
      Win32ResizeDIBSection(&ApplicationData->Buffer, X, Y, Width, Height);
    } break;

    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_KEYDOWN:
    case WM_KEYUP:
    {
      uint8_t VKeyCode = WParam;
      bool WasKeyDown  = (LParam & (1 << 30)) != 0; 
      bool IsKeyDown   = (LParam & (1 << 31)) == 0;
      game_input_controller* NewInput = &ApplicationData->NewInputs.Controllers[0];
      game_input_controller* OldInput = &ApplicationData->OldInputs.Controllers[0];

      if (WasKeyDown != IsKeyDown)
      {
        switch (VKeyCode)
        {
          case VK_UP:
            {
              if (ApplicationData->SoundOutput)
              {
                ApplicationData->SoundOutput->WavePeriod = ApplicationData->SoundOutput->SamplesPerSecond / ApplicationData->SoundOutput->ToneHz;
              }

              Win32ReadXInputButtonState(&OldInput->UpButton, &NewInput->UpButton, IsKeyDown); 
              OutputDebugStringA("VK_UP\n");
            } break;
          case VK_DOWN:
            {
              if (ApplicationData->SoundOutput)
              {
                ApplicationData->SoundOutput->WavePeriod = ApplicationData->SoundOutput->SamplesPerSecond / ApplicationData->SoundOutput->ToneHz;
              }
              Win32ReadXInputButtonState(&OldInput->DownButton, &NewInput->DownButton, IsKeyDown); 
              OutputDebugStringW(L"VK_DOWN\n");
            } break;
          case VK_LEFT:
            {
              Win32ReadXInputButtonState(&OldInput->LeftButton, &NewInput->LeftButton, IsKeyDown); 

              OutputDebugStringA("VK_LEFT\n");

            } break;
          case VK_RIGHT:
            {
              Win32ReadXInputButtonState(&OldInput->RightButton, &NewInput->RightButton, IsKeyDown); 
              OutputDebugStringW(L"VK_RIGHT\n");
            } break;
          case VK_SPACE:
            {
              OutputDebugStringW(L"VK_SPACE\n");
            } break;
          case VK_ESCAPE:
            {
              OutputDebugStringW(L"VK_ESCAPE\n");
            } break;
          case 'W':
            {
              OutputDebugStringW(L"W\n");
            } break;
          case 'A':
            {
              OutputDebugStringW(L"A\n");
            } break;
          case 'S':
            {
              OutputDebugStringW(L"S\n");
            } break;
          case 'D':
            {
              OutputDebugStringW(L"D\n");
            } break;
          case 'Q':
            {
              OutputDebugStringW(L"Q\n");
            } break;
          case 'E':
            {
              OutputDebugStringW(L"E\n");
            } break;
          case VK_F4:
            {
              bool IsAltDown = ((LParam & (1 << 29)) != 0);
              if (IsAltDown)
              {
                Win32AppIsRunning = false;
              }
            } break;
          default:
            {
            } break;
        }
      }
    } break;
    case WM_CLOSE:
    {
      Win32AppIsRunning = false;
    } break;
    case WM_QUIT:
    {
      Win32AppIsRunning = false;
    } break;
    case WM_ACTIVATEAPP:
    {
      OutputDebugStringW(L"WM_ACTIVATEAPP\n");
    } break;
    case WM_CREATE:
    {
      CREATESTRUCTW* WindowCreateInfo = (CREATESTRUCTW*) LParam;
      SetWindowLongPtrW(Window, GWLP_USERDATA, (LONG_PTR) WindowCreateInfo->lpCreateParams);
    } break;
    default:
    {
      Res = DefWindowProc(Window, Msg, WParam, LParam);
    } break;
  }

  return Res;
}


int wWinMain(HINSTANCE hInstance,
  HINSTANCE hPrevInstance,
  LPWSTR     lpCmdLine,
  int       nShowCmd)
{
  LoadXInputDll();
  win32_application_data ApplicationData = {};

  WNDCLASSEXW WindowClass = {};
  WindowClass.cbSize = sizeof(WNDCLASSEXW);
  WindowClass.lpfnWndProc = &Win32WindowProc;
  WindowClass.hInstance = hInstance;
  WindowClass.lpszClassName = L"HandmadeWindowClass";

  if (0 == RegisterClassExW(&WindowClass))
  {
    OutputDebugStringW(L"Can't register window class\n");
    return 1;
  }

  // NOTE (hanasou): We will pass application data and store it when
  // WM_CREATE is called because other messages are handled by application
  // before we enter event loop (WM_SIZE)

  HWND Window = CreateWindowExW(0,
    WindowClass.lpszClassName,
    L"Handmade Hero",
    WS_OVERLAPPEDWINDOW | WS_VISIBLE,
    CW_USEDEFAULT,
    CW_USEDEFAULT,
    CW_USEDEFAULT,
    CW_USEDEFAULT,
    0,
    0,
    hInstance,
    (LPVOID) &ApplicationData 
  );

  if (Window == NULL)
  {
    OutputDebugStringW(L"Can't create window instance\n");
    return 1;
  }

  win32_sound_output SoundOutput;
  SoundOutput.ToneHz = 261;
  SoundOutput.SamplesPerSecond = 48000;
  SoundOutput.BytesPerSample = sizeof (int16_t) * 2;
  SoundOutput.Volume = 4000;
  SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample;
  SoundOutput.SampleRunningIndex = 0;
  SoundOutput.WavePeriod = SoundOutput.SamplesPerSecond / SoundOutput.ToneHz; // (samples/s) / (cycles/s) = samples/cycle
  SoundOutput.Angle = 0.0f;
  bool IsSoundPlaying = false;

  ApplicationData.SecondarySoundBuffer = Win32InitDSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
  Win32ClearSoundBuffer(ApplicationData.SecondarySoundBuffer, &SoundOutput);
  if (SUCCEEDED(ApplicationData.SecondarySoundBuffer->Play(0, 0, DSBPLAY_LOOPING)))
  {
    IsSoundPlaying = true;
    ApplicationData.SoundOutput = &SoundOutput;
  }
  else
  {
    OutputDebugStringW(L"Can't create window instance\n");
  }


  Win32AppIsRunning = true;


  LARGE_INTEGER FrameStartCount, FrameEndCount;
  LARGE_INTEGER ProcessorFrequency;
  QueryPerformanceFrequency(&ProcessorFrequency);

  QueryPerformanceCounter(&FrameStartCount);

  while (Win32AppIsRunning)
  {
    uint64_t ProcessorCounterStart = __rdtsc();
    MSG Msg;

    game_input_controllers* NewInputs = &ApplicationData.NewInputs;
    game_input_controllers* OldInputs = &ApplicationData.OldInputs;

    while (PeekMessageW(&Msg, Window, 0, 0, PM_REMOVE))
    {
      TranslateMessage(&Msg);
      DispatchMessageW(&Msg);

      if (Msg.message == WM_QUIT)
      {
        Win32AppIsRunning = false;
      }
    }

    for (int ControllerIndex = 0; ControllerIndex < XUSER_MAX_COUNT; ++ControllerIndex)
    {
      XINPUT_STATE ControllerState = {};
      if (XInputGetState(ControllerIndex, &ControllerState) != ERROR_SUCCESS)
      {
        continue;
      }
      game_input_controller* OldInput = &OldInputs->Controllers[ControllerIndex];
      game_input_controller* NewInput = &NewInputs->Controllers[ControllerIndex];

      XINPUT_GAMEPAD* Pad = &ControllerState.Gamepad;

      NewInput->IsAnalog = true;

      int16_t StickX = Pad->sThumbLX;
      int16_t StickY = Pad->sThumbLY;

      NewInput->StartX = OldInput->EndX;
      NewInput->StartY = OldInput->EndY;

      if (StickX >= 0)
      {
        NewInput->EndX = (float) StickX / 32767.0f;
      }
      else
      {
        NewInput->EndX = (float) StickX / 32768.0f;
      }

      if (StickY >= 0)
      {
        NewInput->EndY = (float) StickY / 32767.0f;
      }
      else
      {
        NewInput->EndY = (float) StickY / 32768.0f;
      }

      NewInput->MaxX = (NewInput->StartX > NewInput->EndX) ? NewInput->StartX : NewInput->EndX;
      NewInput->MaxY = (NewInput->StartY > NewInput->EndY) ? NewInput->StartY : NewInput->EndY;

      // bool Up    = Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP;
      // bool Down  = Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN;
      // bool Left  = Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT;
      // bool Right = Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT;
      // bool Start = Pad->wButtons & XINPUT_GAMEPAD_START;
      // bool Back  = Pad->wButtons & XINPUT_GAMEPAD_BACK;


      Win32ReadXInputButtonState(&OldInput->LeftButton, &NewInput->LeftButton, Pad->wButtons & XINPUT_GAMEPAD_X);
      Win32ReadXInputButtonState(&OldInput->RightButton, &NewInput->RightButton, Pad->wButtons & XINPUT_GAMEPAD_B);
      Win32ReadXInputButtonState(&OldInput->UpButton, &NewInput->UpButton, Pad->wButtons & XINPUT_GAMEPAD_Y);
      Win32ReadXInputButtonState(&OldInput->DownButton, &NewInput->DownButton, Pad->wButtons & XINPUT_GAMEPAD_A);

      Win32ReadXInputButtonState(&OldInput->LeftShoulderButton, &NewInput->LeftShoulderButton, Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
      Win32ReadXInputButtonState(&OldInput->RightShoulderButton, &NewInput->RightShoulderButton, Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
    }


    RECT ClientRect;
    HDC DeviceContext = GetDC(Window);
    GetClientRect(Window, &ClientRect);
    int WindowWidth = ClientRect.right - ClientRect.left;
    int WindowHeight = ClientRect.bottom - ClientRect.top;

    game_offscreen_buffer GameDrawingBuffer;
    GameDrawingBuffer.Data = ApplicationData.Buffer.Data;
    GameDrawingBuffer.Width = ApplicationData.Buffer.Width;
    GameDrawingBuffer.Height = ApplicationData.Buffer.Height;
    GameDrawingBuffer.Pitch  = ApplicationData.Buffer.Pitch;


    DWORD PlayCursor = 0;
    DWORD WriteCursor = 0;
    local_persist DWORD LastWriteCursor = SoundOutput.SecondaryBufferSize;

    unsigned ByteToLock = 0 ;
    unsigned BytesToWrite = 0;
    int Distance = 0;
    bool SoundIsValid = false;
    LPDIRECTSOUNDBUFFER SoundBuffer = ApplicationData.SecondarySoundBuffer;
    if (SUCCEEDED(SoundBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor)))
    {
      unsigned BytesToWriteUpperBound = (SoundOutput.BytesPerSample * SoundOutput.SamplesPerSecond / 10);

      if (LastWriteCursor == SoundOutput.SecondaryBufferSize)
      {
        LastWriteCursor = WriteCursor;
      }

      if (LastWriteCursor >= WriteCursor)
      {
        ByteToLock = LastWriteCursor;

        Distance = LastWriteCursor - WriteCursor;
        BytesToWrite = 0;
        BytesToWrite = (Distance >= BytesToWriteUpperBound) ? 0 : (BytesToWriteUpperBound - Distance);
      }
      else
      {
        ByteToLock = LastWriteCursor;
        Distance = LastWriteCursor + SoundOutput.SecondaryBufferSize - WriteCursor;
        BytesToWrite = 0;
        BytesToWrite = (Distance >= BytesToWriteUpperBound) ? 0 : (BytesToWriteUpperBound - Distance);
      }

      SoundIsValid = true;
      LastWriteCursor = (ByteToLock + BytesToWrite) % SoundOutput.SecondaryBufferSize;
    }

    int16_t Samples[48000 * 2];
    game_sound_output_buffer GameSoundBuffer = {};

    if (SoundIsValid)
    {
      GameSoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
      GameSoundBuffer.SampleCountToOutput = BytesToWrite / SoundOutput.BytesPerSample;
      GameSoundBuffer.Samples = Samples;
    }

    GameUpdateAndRender(&GameDrawingBuffer, &GameSoundBuffer, NewInputs);

    if (SoundIsValid)   
    {
      Win32FillSoundBuffer(SoundBuffer, &SoundOutput, ByteToLock, BytesToWrite, &GameSoundBuffer);
    }

    Win32UpdateWindow(&ApplicationData.Buffer, DeviceContext, &ClientRect, 0, 0, WindowWidth, WindowHeight);

    ReleaseDC(Window, DeviceContext);



    *OldInputs = *NewInputs;
    *NewInputs = {};

    QueryPerformanceCounter(&FrameEndCount);
    LONGLONG EslapsedTime = (FrameEndCount.QuadPart - FrameStartCount.QuadPart) * 1000 / ProcessorFrequency.QuadPart ;
    int FPS = ProcessorFrequency.QuadPart  / (FrameEndCount.QuadPart - FrameStartCount.QuadPart);
    uint64_t ProcessorCounterEnd = __rdtsc();
    uint64_t ProcessorCounterEslapsed = ProcessorCounterEnd - ProcessorCounterStart;

    Win32PrintPerfInformation(EslapsedTime, FPS, ProcessorCounterEslapsed / 1'000'000);
    FrameStartCount = FrameEndCount;

  }
  return 0;
}
