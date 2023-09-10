#include <windows.h>
#include <cstdint>
#include <cassert>

#include <xinput.h>

#define global_variable static
#define local_persist   static
#define internal_func        static

#define X_INPUT_GET_STATE(name) DWORD WINAPI name (DWORD dwUserIndex, XINPUT_STATE* pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
  return 0;
}

#define X_INPUT_SET_STATE(name) DWORD WINAPI name (DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
  return 0;
}

#define XInputGetState XInputGetState_
#define XInputSetState XInputSetState_
global_variable x_input_get_state* XInputGetState = &XInputGetStateStub;
global_variable x_input_set_state* XInputSetState = &XInputSetStateStub;

internal_func void
LoadXInputDll()
{
  local_persist const wchar_t DllName [] = L"xinput1_4.dll";

  HMODULE Handle = LoadLibraryW(L"xinput1_4.dll");
  if (Handle != NULL)
  {
    XInputGetState = (x_input_get_state*) GetProcAddress(Handle, "XInputGetState");
    XInputSetState = (x_input_set_state*) GetProcAddress(Handle, "XInputSetState");
  }
  else
  {
    OutputDebugStringW(L"Can't load ");
    OutputDebugStringW(DllName);
    OutputDebugStringW(L"\n");
  }
}


global_variable bool       Win32AppIsRunning;
global_variable int        BytesPerPixel = 4;

struct win32_offscreen_buffer
{
  BITMAPINFO BitMapInfo;
  int        Width;
  int        Height;
  void*      Data;
};

struct win32_window_size
{
  int Width;
  int Height;
};

struct win32_application_data
{
  win32_offscreen_buffer Buffer;
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

      *Pixel++ =  (Green << 8) | Blue;

    }

    Rows += BytesPerPixel * Buffer->Width;
  }
}


internal_func void
Win32ResizeDIBSection(win32_offscreen_buffer* Buffer, int X, int Y, int Width, int Height)
{
  if (Buffer->Data)
  {
    VirtualFree(Buffer->Data, 0, MEM_RELEASE);
    Buffer->Data = NULL;
  }

  int BitsPerPixel = 32;
  Buffer->Width = Width;
  Buffer->Height = Height;

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
              OutputDebugStringW(L"VK_UP\n");
            } break;
          case VK_DOWN:
            {
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

  Win32AppIsRunning = true;
  int XOffset = 0;
  int YOffset = 0;

  while (Win32AppIsRunning)
  {
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

    RenderWeirdRectangle(&ApplicationData.Buffer, XOffset, YOffset);
    Win32UpdateWindow(&ApplicationData.Buffer, DeviceContext, &ClientRect, 0, 0, WindowWidth, WindowHeight);

    ReleaseDC(Window, DeviceContext);

    XOffset++;
    YOffset += 2;


  }

  return 0;
}
