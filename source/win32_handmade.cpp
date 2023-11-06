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

#define KILOBYTES(n) 1024l * (n)
#define MEGABYTES(n) 1024l * KILOBYTES(n)
#define GIGABYTES(n) 1024l * MEGABYTES(n)

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
global_variable LARGE_INTEGER ProcessorFrequency;

struct win32_offscreen_buffer
{
  BITMAPINFO BitMapInfo;
  void*      Data;
  int        Width;
  int        Height;
  int        Pitch;
  int        BytesPerPixel;
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
  game_inputs NewInputs;
  game_inputs OldInputs;
};


internal_func void
Win32PrintPerfInformation(float EslapsedTime, int FPS, unsigned long long ProcessorCounterEslapsed)
{
  if (ShouldDisplayDebugInfo)
  {
    char TimeDebugStr[128];
    std::snprintf(TimeDebugStr, 128, "Frame: %fms, FPS: %d, ProcessorCounter: %lluM\n", EslapsedTime, FPS, ProcessorCounterEslapsed);
    OutputDebugStringA(TimeDebugStr);
  }
}


internal_func bool
PlatformReadEntireFile(char const* FileName, debug_file_result* Result)
{
  HANDLE FileHandle = CreateFileA(FileName,
                                  GENERIC_READ,
                                  FILE_SHARE_READ,
                                  0,
                                  OPEN_EXISTING,
                                  FILE_ATTRIBUTE_NORMAL,
                                  0);
  if (FileHandle != INVALID_HANDLE_VALUE)
  {
    LARGE_INTEGER FileSize;
    if (GetFileSizeEx(FileHandle, &FileSize))
    {
      uint64_t Size = FileSize.QuadPart;
      DWORD SizeTruncated = SafeTruncateNumber(Size);

      void* Buffer = VirtualAlloc(0, SizeTruncated, MEM_COMMIT, PAGE_READWRITE);
      if (Buffer)
      {
        DWORD BytesRead = 0;
        if (ReadFile(FileHandle, Buffer, SizeTruncated, &BytesRead, 0) && BytesRead == SizeTruncated)
        {
          Result->Size = BytesRead;
          Result->Memory = Buffer;
          CloseHandle(FileHandle);
          return true;
        }
        else
        {
          VirtualFree(Buffer, 0, MEM_RELEASE);
        }
      }
    }

    CloseHandle(FileHandle);
  }
  return false;
}

internal_func bool
PlatformWriteEntireFile(char const* FileName, void const* Buffer, uint64_t Size) 
{
  HANDLE FileHandle = CreateFileA(FileName,
                                  GENERIC_WRITE,
                                  0,
                                  0,
                                  CREATE_ALWAYS,
                                  FILE_ATTRIBUTE_NORMAL,
                                  0);

  if (FileHandle != INVALID_HANDLE_VALUE)
  {
    DWORD BytesToWrite = SafeTruncateNumber(Size);
    DWORD BytesWritten = 0;

    if (WriteFile(FileHandle, Buffer, BytesToWrite, &BytesWritten, 0) && BytesWritten == BytesToWrite)
    {
      CloseHandle(FileHandle);
      return true;
    }
    CloseHandle(FileHandle);
  }
  return false;
}

internal_func void 
PlatformFreeFileMemory(debug_file_result* File)
{
  VirtualFree(File->Memory, 0, MEM_RELEASE);
}

internal_func void
Win32ProcessXInputButtonState(DWORD ButtonsState,
  DWORD ButtonBit,
  game_button_state* OldState,
  game_button_state* NewState)
{
  NewState->EndedDown = ((ButtonsState & ButtonBit) == ButtonBit); // TODO (hanasou): Verify this is to allow only one button pressed at a time
  NewState->HalfTransitionCount = (NewState->EndedDown != OldState->EndedDown) ? 1 : 0;
}

internal_func void
Win32ProcessXInputStickValue(SHORT Value, SHORT DeadZoneValue, float* NormalizedValue)
{
  if (Value >= DeadZoneValue)
  {
    *NormalizedValue = (float) Value / (32767.0f - (float) DeadZoneValue);
  }
  else if (Value <= -DeadZoneValue)
  {
    *NormalizedValue = (float) Value / (32768.0f - (float) DeadZoneValue);
  }
  else
  {
    *NormalizedValue = 0.0f;
  }
}

internal_func void
Win32ReadKeyboardButtonState(game_button_state*, game_button_state* NewState, int ButtonBit)
{
  HANDMADE_ASSERT(NewState->EndedDown != (bool) (ButtonBit));
  NewState->EndedDown = (bool) ButtonBit;
  NewState->HalfTransitionCount += 1;
}


internal_func void
Win32DebugDrawSingleVerticalLine(win32_offscreen_buffer* Buffer,
  int X, int Y,
  int Height,
  int PaddingX, int PaddingY,
  uint32_t Color)
{
  int BufferWidth = Buffer->Width;
  int BufferHeight = Buffer->Height;
  Y = Y + PaddingY; // move Y to padding
  X += PaddingX;

  int MaxY = (Y + Height) >= (BufferHeight - PaddingY)
    ? (BufferHeight - PaddingY)
    : (Y + Height);
  if (X >= BufferWidth - PaddingX) return; // X fall out of screen

  for (int YY = Y; YY < MaxY; ++YY)
  {
    uint8_t* Row = (uint8_t*)(Buffer->Data) + YY * Buffer->Pitch;
    uint32_t* Pixel = (uint32_t*) (Row + X * Buffer->BytesPerPixel);
    *Pixel = Color;
  }
}

internal_func void
Win32DebugSyncDisplay(win32_offscreen_buffer* Buffer,
  win32_sound_output* SoundInfo,
  LPDIRECTSOUNDBUFFER SoundBuffer,
  DWORD* LastCursorPositions,
  int LastCursorPositionCount,
  int LastCursorPositionIndex)
{
  int SecondaryBufferSize = SoundInfo->SecondaryBufferSize;
  int SoundBytesPerPixel = SecondaryBufferSize / Buffer->Width;

  for (int CursorIndex = 0; CursorIndex < LastCursorPositionCount; CursorIndex += 2)
  {
    int PlayCursorX = LastCursorPositions[CursorIndex] / SoundBytesPerPixel;
    int WriteCursorX = LastCursorPositions[CursorIndex + 1] / SoundBytesPerPixel;
    int PaddingX = 20;
    int PaddingY = 20;
    int Y = 0;
    int Height = Buffer->Height;

    Win32DebugDrawSingleVerticalLine(Buffer, PlayCursorX, Y, Height, PaddingX, PaddingY, 0xFFFFFFFF);
    Win32DebugDrawSingleVerticalLine(Buffer, WriteCursorX, Y, Height, PaddingX, PaddingY, 0xFF00FFFF);
  }
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
Win32ResizeDIBSection(win32_offscreen_buffer* Buffer, int Width, int Height)
{
  assert(Width >= 0 && Height >= 0 && Buffer);
  if (Buffer->Data)
  {
    VirtualFree(Buffer->Data, 0, MEM_RELEASE);
    Buffer->Data = NULL;
  }

  int16_t BitsPerPixel = 32;
  Buffer->Width = Width;
  Buffer->Height = Height;
  Buffer->Pitch = BytesPerPixel * Buffer->Width;
  Buffer->BytesPerPixel = BytesPerPixel;

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
                  RECT* ClientRect)
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

  int16_t NumChannels = 2;
  int16_t BitsPerSample = 16;

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
    case WM_CLOSE:
    case WM_QUIT:
    {
      Win32AppIsRunning = false;
      break;
    }
    case WM_PAINT:
    {
      PAINTSTRUCT PaintInfo;
      BeginPaint(Window, &PaintInfo);
      RECT ClientRect;
      GetClientRect(Window, &ClientRect);
      Win32UpdateWindow(&ApplicationData->Buffer, PaintInfo.hdc, &ClientRect);
      EndPaint(Window, &PaintInfo);

    } break;
    case WM_SIZE:
    {
      RECT ClientRect;
      GetClientRect(Window, &ClientRect);
      int Width = ClientRect.right - ClientRect.left;
      int Height = ClientRect.bottom - ClientRect.top;
      Win32ResizeDIBSection(&ApplicationData->Buffer, Width, Height);
    } break;

    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_KEYDOWN:
    case WM_KEYUP:
    {
      HANDMADE_ASSERT(false); // These messages are handled externally in winMain()
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

internal_func void
Win32ProcessPendingMessages(game_input_controller* NewController, game_input_controller* OldController, win32_sound_output* SoundOutput)
{
  MSG Message;
  while (PeekMessageW(&Message, 0, 0, 0, PM_REMOVE))
  {
    switch (Message.message)
    {
      case WM_SYSKEYDOWN:
      case WM_SYSKEYUP:
      case WM_KEYDOWN:
      case WM_KEYUP:
        {
          WPARAM WParam = Message.wParam;
          LPARAM LParam = Message.lParam;

          uint8_t VKeyCode = (uint8_t) WParam;
          bool WasKeyDown  = (LParam & (1 << 30)) != 0; 
          bool IsKeyDown   = (LParam & (1 << 31)) == 0;
          if (WasKeyDown != IsKeyDown)
          {
            switch (VKeyCode)
            {
              case VK_UP:
                {
                  if (SoundOutput)
                  {
                    SoundOutput->WavePeriod = SoundOutput->SamplesPerSecond / SoundOutput->ToneHz;
                  }

                  Win32ReadKeyboardButtonState(&OldController->ActionUp, &NewController->ActionUp, IsKeyDown); 
                } break;
              case VK_DOWN:
                {
                  if (SoundOutput)
                  {
                    SoundOutput->WavePeriod = SoundOutput->SamplesPerSecond / SoundOutput->ToneHz;
                  }
                  Win32ReadKeyboardButtonState(&OldController->ActionDown, &NewController->ActionDown, IsKeyDown); 
                } break;
              case VK_LEFT:
                {
                  Win32ReadKeyboardButtonState(&OldController->ActionLeft, &NewController->ActionLeft, IsKeyDown); 
                } break;
              case VK_RIGHT:
                {
                  Win32ReadKeyboardButtonState(&OldController->ActionRight, &NewController->ActionRight, IsKeyDown); 
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
                  Win32ReadKeyboardButtonState(&OldController->MoveUp, &NewController->MoveUp, IsKeyDown); 
                } break;
              case 'A':
                {
                  Win32ReadKeyboardButtonState(&OldController->MoveLeft, &NewController->MoveLeft, IsKeyDown); 
                } break;
              case 'S':
                {
                  Win32ReadKeyboardButtonState(&OldController->MoveDown, &NewController->MoveDown, IsKeyDown); 
                } break;
              case 'D':
                {
                  Win32ReadKeyboardButtonState(&OldController->MoveRight, &NewController->MoveRight, IsKeyDown); 
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
      default:
        {
          TranslateMessage(&Message);
          DispatchMessageW(&Message);
        }
    }
  }
}


internal_func LARGE_INTEGER
Win32GetWallClock()
{
  LARGE_INTEGER Value;
  QueryPerformanceCounter(&Value);
  return Value;
}
internal_func float
Win32GetTimeElapsedMs(LARGE_INTEGER const* From, LARGE_INTEGER const* To)
{
  return (To->QuadPart - From->QuadPart) * 1000.0f / (float) ProcessorFrequency.QuadPart;
}



int wWinMain(HINSTANCE hInstance,
  HINSTANCE /* hPrevInstance */,
  LPWSTR    /* lpCmdLine */,
  int       /*nShowCmd*/)
{
  LoadXInputDll();
  win32_application_data ApplicationData = {};

  if (TIMERR_NOCANDO == timeBeginPeriod(1))
  {
    OutputDebugStringA("Get set resolution time\n");
    return 1;
  }

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


  game_memory GameMemory = {};
  GameMemory.PermanentStorageSize = MEGABYTES(32);
  GameMemory.TransientStorageSize = MEGABYTES(1);

  uint64_t TotalGameMemoryBytes = GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;
#ifdef HANDMADE_INTERNAL
  // Fix address to reload / debug
  void* GameFixedAddress = (void*) 0x00000000bbdf0000ll;
#else
  void* GameFixedAddress = (void*) 0x0ll;
#endif
  void* GameMemoryBuffer = VirtualAlloc(GameFixedAddress, TotalGameMemoryBytes, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
  GameMemory.PermanentStorage = GameMemoryBuffer;
  GameMemory.TransientStorage = (char*) GameMemory.PermanentStorage + GameMemory.PermanentStorageSize;


  Win32AppIsRunning = true;


  QueryPerformanceFrequency(&ProcessorFrequency);
  LARGE_INTEGER FrameStartTime = Win32GetWallClock();
  LARGE_INTEGER FrameEndTime = FrameStartTime;

  int   ExpectedFramesPerSecond = 30;
  float ExpectedFrameTime = (1000.0f /  ExpectedFramesPerSecond);


#if HANDMADE_INTERNAL
  int const LastCursorPositionCount = 30; // 30 locations stored for 2 cursors
  DWORD LastCursorPositions[LastCursorPositionCount] = {};
  int LastCursorPositionIndex = 0;
#endif

  while (Win32AppIsRunning)
  {
    uint64_t ProcessorCounterStart = __rdtsc();

    game_inputs* NewInputs = &ApplicationData.NewInputs;
    game_inputs* OldInputs = &ApplicationData.OldInputs;

    game_input_controller* OldKeyboardController = &OldInputs->Controllers[GAMEINPUT_KEYBOARD_INDEX];
    game_input_controller* NewKeyboardController = &NewInputs->Controllers[GAMEINPUT_KEYBOARD_INDEX];
    game_input_controller ZeroController = {};
    *NewKeyboardController = ZeroController;
    
    for (unsigned ButtonIndex = 0;
      ButtonIndex < ARRAY_SIZE(NewKeyboardController->Buttons);
      ++ButtonIndex)
    {
      NewKeyboardController->Buttons[ButtonIndex].EndedDown = OldKeyboardController->Buttons[ButtonIndex].EndedDown;
    }

    Win32ProcessPendingMessages(NewKeyboardController, OldKeyboardController, &SoundOutput);

    DWORD MaxControllerCount = (XUSER_MAX_COUNT >= ARRAY_SIZE(NewInputs->Controllers)) ? ARRAY_SIZE(NewInputs->Controllers) : XUSER_MAX_COUNT;
    for (unsigned ControllerIndex = 0; ControllerIndex < MaxControllerCount; ++ControllerIndex)
    {
      unsigned OurControllerIndex = ControllerIndex + 1;
      XINPUT_STATE ControllerState = {};
      if (XInputGetState(ControllerIndex, &ControllerState) != ERROR_SUCCESS)
      {
        continue;
      }
      game_input_controller* OldController = &OldInputs->Controllers[OurControllerIndex];
      game_input_controller* NewController = &NewInputs->Controllers[OurControllerIndex];

      XINPUT_GAMEPAD* Pad = &ControllerState.Gamepad;

      NewController->IsAnalog = true;

      // Read and normalize thumb stick value
      int16_t StickX = Pad->sThumbLX;
      int16_t StickY = Pad->sThumbLY;
      float NormalizedStickX;
      float NormalizedStickY;
      Win32ProcessXInputStickValue(StickX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE, &NormalizedStickX);
      Win32ProcessXInputStickValue(StickY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE, &NormalizedStickY);

      // if any DPAD is pressed, we override stick value to that button
      bool DpadUp = Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP;
      bool DpadDown = Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN;
      bool DpadLeft = Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT;
      bool DpadRight = Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT;

      if (DpadUp)
      {
        NormalizedStickY = 1.0f;
      }
      if (DpadDown)
      {
        NormalizedStickY = -1.0f;
      } 
      if (DpadRight)
      {
        NormalizedStickX = 1.0f;
      }
      if (DpadLeft)
      {
        NormalizedStickX = -1.0f;
      }

      // We can reuse Win32ProcessXInputButtonState with Stick value
      // based on value we can fake the ButtonsState to be either 0 or 1
      // the ButtonBit is always 1, then the two value can AND together
      // and ButtonsState value dominates

      Win32ProcessXInputButtonState((NormalizedStickX > 0.5f) ? 1 : 0,
        1,
        &OldController->MoveRight,
        &NewController->MoveRight);
      Win32ProcessXInputButtonState((NormalizedStickX < -0.5f) ? 1 : 0,
        1,
        &OldController->MoveLeft,
        &NewController->MoveLeft);
      Win32ProcessXInputButtonState((NormalizedStickY > 0.5f) ? 1 : 0,
        1,
        &OldController->MoveUp,
        &NewController->MoveUp);
      Win32ProcessXInputButtonState((NormalizedStickY < -0.5f) ? 1 : 0,
        1,
        &OldController->MoveDown,
        &NewController->MoveDown);


      Win32ProcessXInputButtonState(Pad->wButtons,
        Pad->wButtons & XINPUT_GAMEPAD_X,
        &OldController->ActionLeft,
        &NewController->ActionLeft);
      Win32ProcessXInputButtonState(Pad->wButtons,
        XINPUT_GAMEPAD_B,
        &OldController->ActionRight,
        &NewController->ActionRight);
      Win32ProcessXInputButtonState(Pad->wButtons,
        XINPUT_GAMEPAD_Y,
        &OldController->ActionUp,
        &NewController->ActionUp);
      Win32ProcessXInputButtonState(Pad->wButtons,
        XINPUT_GAMEPAD_A,
        &OldController->ActionDown,
        &NewController->ActionDown);

      Win32ProcessXInputButtonState(Pad->wButtons,
        XINPUT_GAMEPAD_LEFT_SHOULDER,
        &OldController->LeftShoulderButton,
        &NewController->LeftShoulderButton);
      Win32ProcessXInputButtonState(Pad->wButtons,
        XINPUT_GAMEPAD_RIGHT_SHOULDER,
        &OldController->RightShoulderButton,
        &NewController->RightShoulderButton);
      Win32ProcessXInputButtonState(Pad->wButtons,
        XINPUT_GAMEPAD_START,
        &OldController->StartButton,
        &NewController->StartButton);
      Win32ProcessXInputButtonState(Pad->wButtons,
        XINPUT_GAMEPAD_BACK,
        &OldController->BackButton,
        &NewController->StartButton);
    }


    RECT ClientRect;
    HDC DeviceContext = GetDC(Window);
    GetClientRect(Window, &ClientRect);

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
        BytesToWrite = ((unsigned)Distance >= BytesToWriteUpperBound) ? 0 : (BytesToWriteUpperBound - Distance);
      }
      else
      {
        ByteToLock = LastWriteCursor;
        Distance = LastWriteCursor + SoundOutput.SecondaryBufferSize - WriteCursor;
        BytesToWrite = 0;
        BytesToWrite = ((unsigned) Distance >= BytesToWriteUpperBound) ? 0 : (BytesToWriteUpperBound - Distance);
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

    LARGE_INTEGER CurrentTime = Win32GetWallClock();
    float EslapsedTime = Win32GetTimeElapsedMs(&FrameStartTime, &CurrentTime);
    NewInputs->Timers.EslapsedTime = EslapsedTime;

    GameUpdateAndRender(&GameDrawingBuffer, &GameSoundBuffer, NewInputs, &GameMemory);

    if (SoundIsValid)   
    {
      Win32FillSoundBuffer(SoundBuffer, &SoundOutput, ByteToLock, BytesToWrite, &GameSoundBuffer);
    }


    // Swap inputs, NewInputs become OldInputs, NewsInput can be clear or whatever
    *OldInputs = *NewInputs;
    *NewInputs = {};

    CurrentTime = Win32GetWallClock();
    EslapsedTime = Win32GetTimeElapsedMs(&FrameStartTime, &CurrentTime);

    if (ExpectedFrameTime > EslapsedTime)
    {
      while (ExpectedFrameTime > EslapsedTime)
      {
        Sleep(1);
        CurrentTime = Win32GetWallClock();
        EslapsedTime = Win32GetTimeElapsedMs(&FrameStartTime, &CurrentTime);
      }
    }
    else
    {
      // TODO (hanasou): We missed a frame, should put a log here
    }

#if HANDMADE_INTERNAL
    Win32DebugSyncDisplay(&ApplicationData.Buffer, &SoundOutput, SoundBuffer, LastCursorPositions, LastCursorPositionCount, LastCursorPositionIndex);
#endif
    Win32UpdateWindow(&ApplicationData.Buffer, DeviceContext, &ClientRect);
    ReleaseDC(Window, DeviceContext);



#if HANDMADE_INTERNAL
    {
      SoundBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor);
      LastCursorPositions[LastCursorPositionIndex++] = PlayCursor;
      LastCursorPositions[LastCursorPositionIndex++] = WriteCursor;
      (void) LastCursorPositionIndex;

      if (LastCursorPositionIndex >= LastCursorPositionCount) LastCursorPositionIndex = 0;
    }
#endif


    FrameEndTime = Win32GetWallClock();
    CurrentTime = FrameEndTime;
    EslapsedTime = Win32GetTimeElapsedMs(&FrameStartTime, &CurrentTime);

    int FPS = (int) (ProcessorFrequency.QuadPart  / (FrameEndTime.QuadPart - FrameStartTime.QuadPart));
    uint64_t ProcessorCounterEnd = __rdtsc();
    uint64_t ProcessorCounterEslapsed = ProcessorCounterEnd - ProcessorCounterStart;

    Win32PrintPerfInformation(EslapsedTime, FPS, ProcessorCounterEslapsed / 1'000'000);
    FrameStartTime = FrameEndTime;
  }
  return 0;
}
