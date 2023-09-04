#include <windows.h>
#include <cstdint>
#include <cassert>
#define global_variable static
#define local_persist   static
#define internal_func        static

global_variable bool       Win32AppIsRunning;
global_variable BITMAPINFO BitMapInfo;
global_variable int        ScreenWidth;
global_variable int        ScreenHeight;
global_variable void*      BitMapMemory;


internal_func void
RenderWeirdRectangle(int XOffset, int YOffset)
{
  int BytesPerPixel = 4;
  uint8_t* Rows = (uint8_t*) BitMapMemory;
  for (int Y = 0;
       Y < ScreenHeight;
       ++Y)
  {
    uint32_t* Pixel = (uint32_t*) Rows;
    for (int X = 0;
         X < ScreenWidth;
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

    Rows += BytesPerPixel * ScreenWidth;
  }
}


internal_func void
Win32ResizeDIBSection(int X, int Y, int Width, int Height)
{
  if (BitMapMemory)
  {
    VirtualFree(BitMapMemory, 0, MEM_RELEASE);
  }

  int BytesPerPixel = 4;
  int BitsPerPixel = 32;
  ScreenWidth = Width;
  ScreenHeight = Height;

  BitMapInfo.bmiHeader.biSize   = sizeof(BitMapInfo.bmiHeader);
  BitMapInfo.bmiHeader.biWidth  = ScreenWidth;
  BitMapInfo.bmiHeader.biHeight = -ScreenHeight;
  BitMapInfo.bmiHeader.biPlanes  = 1;
  BitMapInfo.bmiHeader.biBitCount = BitsPerPixel;
  BitMapInfo.bmiHeader.biCompression = BI_RGB;

  BitMapMemory = VirtualAlloc(0,
                              BytesPerPixel * ScreenWidth * ScreenHeight,
                              MEM_COMMIT,
                              PAGE_READWRITE);
  assert(BitMapMemory);

  RenderWeirdRectangle(0, 0);
}

internal_func void
Win32UpdateWindow(HDC DeviceContext, RECT* ClientRect, int X, int Y, int Width, int Height)
{
  StretchDIBits(DeviceContext,
  /*
                X, Y, Width, Height,
                X, Y, Width, Height,
   */
                0, 0, ClientRect->right - ClientRect->left, ClientRect->bottom - ClientRect->top,
                0, 0, ScreenWidth, ScreenHeight,
                BitMapMemory,
                &BitMapInfo,
                DIB_RGB_COLORS, SRCCOPY);
}

internal_func LRESULT
Win32WindowProc(HWND Window,
                UINT    Msg,
                WPARAM  WParam,
                LPARAM  LParam
)
{
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
      Win32UpdateWindow(PaintInfo.hdc, &ClientRect, X, Y, Width, Height);
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
      Win32ResizeDIBSection(X, Y, Width, Height);
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
    0
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


    RECT ClientRect;
    HDC DeviceContext = GetDC(Window);
    GetClientRect(Window, &ClientRect);
    int WindowWidth = ClientRect.right - ClientRect.left;
    int WindowHeight = ClientRect.bottom - ClientRect.top;

    RenderWeirdRectangle(XOffset, YOffset);
    Win32UpdateWindow(DeviceContext, &ClientRect, 0, 0, WindowWidth, WindowHeight);

    ReleaseDC(Window, DeviceContext);

    XOffset++;


  }

  return 0;
}
