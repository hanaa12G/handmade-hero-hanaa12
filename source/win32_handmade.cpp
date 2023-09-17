#include <windows.h>
#include <cstdint>
#include <cassert>
#include <cmath>
#include <cstdio>

#include <xinput.h>
#include <dsound.h>


#include "handmade.cpp" 
#define PI 3.14159265359f

#define global_variable static
#define local_persist   static
#define internal_func   static

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
};


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
Win32FillSoundBuffer(LPDIRECTSOUNDBUFFER SoundBuffer, win32_sound_output* SoundOutput, unsigned ByteToLock, unsigned BytesToWrite)
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

    float AngleStep = (2 * PI / (float) SoundOutput->WavePeriod);

    int Region1SampleSize = Region1Size / SoundOutput->BytesPerSample;
    int16_t* Sample = (int16_t*) Region1;
    for (int Region1Index = 0;
        Region1Index < Region1SampleSize;
        ++Region1Index)
    {
      float t = SoundOutput->Angle; 

      int16_t SampleValue = (int16_t) (sinf(t) * SoundOutput->Volume);

      *Sample++ = SampleValue;
      *Sample++ = SampleValue;

      SoundOutput->Angle += AngleStep;
      SoundOutput->SampleRunningIndex += 1;

      // NOTE (hanasou): When Angle is too large the floating point become insignificant, which
      // cause sound shift a bit
      // Adjust value down to always less than 2 PI
      if (SoundOutput->Angle > 2 * PI) {
        SoundOutput->Angle -= 2 * PI;
      }
    }

    Sample = (int16_t*) Region2;
    int Region2SampleSize = Region2Size / SoundOutput->BytesPerSample;
    for (int Region2Index = 0;
        Region2Index < Region2SampleSize;
        ++Region2Index)
    {
      float t = SoundOutput->Angle; 

      // sprintf(tmp, "%f %f\n", t, sinf(t));
      // OutputDebugStringA(tmp);

      int16_t SampleValue = (int16_t) (sinf(t) * SoundOutput->Volume);
      *Sample++ = SampleValue;
      *Sample++ = SampleValue;

      SoundOutput->Angle += AngleStep;
      SoundOutput->SampleRunningIndex += 1;

      // NOTE (hanasou): When Angle is too large the floating point become insignificant, which
      // cause sound shift a bit
      // Adjust value down to always less than 2 PI
      if (SoundOutput->Angle > 2 * PI) {
        SoundOutput->Angle -= 2 * PI;
      }
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

      if (WasKeyDown != IsKeyDown)
      {
        switch (VKeyCode)
        {
          case VK_UP:
            {
              if (ApplicationData->SoundOutput)
              {
                ApplicationData->SoundOutput->ToneHz += 10;
                ApplicationData->SoundOutput->WavePeriod = ApplicationData->SoundOutput->SamplesPerSecond / ApplicationData->SoundOutput->ToneHz;
              }
              OutputDebugStringW(L"VK_UP\n");
            } break;
          case VK_DOWN:
            {
              if (ApplicationData->SoundOutput)
              {
                ApplicationData->SoundOutput->ToneHz -= 10;
                ApplicationData->SoundOutput->WavePeriod = ApplicationData->SoundOutput->SamplesPerSecond / ApplicationData->SoundOutput->ToneHz;
              }
              OutputDebugStringW(L"VK_DOWN\n");
            } break;
          case VK_LEFT:
            {
              OutputDebugStringW(L"VK_LEFT\n");
            } break;
          case VK_RIGHT:
            {
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


internal_func
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
  Win32FillSoundBuffer(ApplicationData.SecondarySoundBuffer, &SoundOutput, 0, SoundOutput.SecondaryBufferSize / 10);
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
  int XOffset = 0;
  int YOffset = 0;



  LARGE_INTEGER FrameStartCount, FrameEndCount;
  LARGE_INTEGER ProcessorFrequency;
  QueryPerformanceFrequency(&ProcessorFrequency);

  QueryPerformanceCounter(&FrameStartCount);

  while (Win32AppIsRunning)
  {
    uint64_t ProcessorCounterStart = __rdtsc();
    MSG Msg;
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

      XINPUT_GAMEPAD* Pad = &ControllerState.Gamepad;

      bool Up    = Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP;
      bool Down  = Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN;
      bool Left  = Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT;
      bool Right = Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT;

      bool Start = Pad->wButtons & XINPUT_GAMEPAD_START;
      bool Back  = Pad->wButtons & XINPUT_GAMEPAD_BACK;

      bool LeftShoulder  = Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER;
      bool RightShoulder = Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER;

      bool AButton = Pad->wButtons & XINPUT_GAMEPAD_A;
      bool BButton = Pad->wButtons & XINPUT_GAMEPAD_B;
      bool XButton = Pad->wButtons & XINPUT_GAMEPAD_X;
      bool YButton = Pad->wButtons & XINPUT_GAMEPAD_Y;

      int16_t StickX = Pad->sThumbLX;
      int16_t StickY = Pad->sThumbLY;
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

    GameUpdateAndRender(&GameDrawingBuffer, XOffset, YOffset);

    if (IsSoundPlaying)   
    {
      LPDIRECTSOUNDBUFFER SoundBuffer = ApplicationData.SecondarySoundBuffer;

      unsigned ByteToLock = (SoundOutput.SampleRunningIndex * SoundOutput.BytesPerSample) % SoundOutput.SecondaryBufferSize;
      unsigned BytesToWrite = 0;

      DWORD PlayCursor;
      DWORD WriteCursor;
      if (SUCCEEDED(SoundBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor)))
      {
        unsigned BytesToWriteUpperBound = (SoundOutput.BytesPerSample * SoundOutput.SamplesPerSecond / 10); // we only want to write 1/10 seconds ahead

        if (ByteToLock == PlayCursor)
        {
          // This is expected to not likely to happen, this case we don't have any extra space so we don't write anything
          BytesToWrite = 0;
        }
        else if (ByteToLock > PlayCursor)
        {
          if (ByteToLock - PlayCursor > BytesToWriteUpperBound)
          {
            // TODO (hanasou) Better way to fix this
            // We is too far ahead so we won't write anything 

            BytesToWrite = 0;
          }
          else
          {
            //  We have two regions to write [BytesToWrite, SecondaryBufferSize) and [0, PlayCursor), however we will calculate maximal size we can write
            BytesToWrite = SoundOutput.SecondaryBufferSize - ByteToLock;
            BytesToWrite += PlayCursor;
          }
        }
        else
        {
          unsigned Distance = (SoundOutput.SecondaryBufferSize - PlayCursor) + ByteToLock;
          if (Distance > BytesToWriteUpperBound)
          {
            // TODO (hanasou) Better way to fix this
            // We is too far ahead so we won't write anything 

            BytesToWrite = 0;
          }
          else
          {
            // We have one region to write, here we calculate maximal size
            // we can write
            BytesToWrite = PlayCursor - ByteToLock;
          }
        }


        // char tmp[100];
        // sprintf(tmp, "%u %d:%u %u %u\n", SoundOutput.SampleRunningIndex, PlayCursor, ByteToLock, BytesToWrite, BytesToWriteUpperBound);
        // OutputDebugStringA(tmp);

        // Round down if we write too much
        if (BytesToWrite > BytesToWriteUpperBound)
        {
          BytesToWrite = BytesToWriteUpperBound;
        }

        Win32FillSoundBuffer(SoundBuffer, &SoundOutput, ByteToLock, BytesToWrite);
      }
      else
      {
        // TODO (hanasou): Diagnostic
      }
    }

    Win32UpdateWindow(&ApplicationData.Buffer, DeviceContext, &ClientRect, 0, 0, WindowWidth, WindowHeight);

    ReleaseDC(Window, DeviceContext);

    XOffset++;
    YOffset += 2;

    QueryPerformanceCounter(&FrameEndCount);
    LONGLONG EslapsedTime = (FrameEndCount.QuadPart - FrameStartCount.QuadPart) * 1000 / ProcessorFrequency.QuadPart ;
    int FPS = ProcessorFrequency.QuadPart  / (FrameEndCount.QuadPart - FrameStartCount.QuadPart);
    uint64_t ProcessorCounterEnd = __rdtsc();
    uint64_t ProcessorCounterEslapsed = ProcessorCounterEnd - ProcessorCounterStart;

    char TimeDebugStr[128];
    std::snprintf(TimeDebugStr, 128, "Frame: %lldms, FPS: %d, ProceessorCounter: %lluM\n", EslapsedTime, FPS, ProcessorCounterEslapsed / 1000000);

    OutputDebugStringA(TimeDebugStr);

    FrameStartCount = FrameEndCount;
  }
  return 0;
}
