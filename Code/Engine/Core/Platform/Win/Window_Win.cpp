#include <Core/CorePCH.h>

#if W_ENABLED(W_PLATFORM_WINDOWS_DESKTOP) && W_DISABLED(W_SUPPORTS_GLFW)

#  include <Core/System/Window.h>
#  include <Foundation/Basics.h>
#  include <Foundation/Logging/Log.h>
#  include <Foundation/Platform/Win/Utils/IncludeWindows.h>
#  include <Foundation/System/SystemInformation.h>

static LRESULT CALLBACK WWindowsMessageFuncTrampoline(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  WWindowWin* pWindow = reinterpret_cast<WWindowWin*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));

  if (pWindow != nullptr && pWindow->IsInitialized())
  {
    if (auto pInput = WDynamicCast<WInputDeviceMouseKeyboard_Win*>(pWindow->GetInputDevice()))
    {
      pInput->WindowMessage(msg, wparam, lparam);
    }

    switch (msg)
    {
      case WM_CLOSE:
        pWindow->OnClickClose();
        return 0;

      case WM_SETFOCUS:
        pWindow->OnFocus(true);
        return 0;

      case WM_KILLFOCUS:
        pWindow->OnFocus(false);
        return 0;

      case WM_SIZE:
      {
        WSizeU32 size(LOWORD(lparam), HIWORD(lparam));
        pWindow->OnVisibleChange(wparam != SIZE_MINIMIZED);
        if (size.width > 0 && size.height > 0)
          pWindow->OnResize(size);
      }
      break;

      case WM_SYSKEYDOWN:
      {
        // filter this message out, otherwise pressing ALT will give focus to the system menu, locking out other actions
        // until ALT is pressed again, which is typically not desired
        return 0;
      }

      case WM_MOVE:
      {
        pWindow->OnWindowMove((int)(short)LOWORD(lparam), (int)(short)HIWORD(lparam));
      }
      break;
    }

    pWindow->OnWindowMessage(WMinWindows::FromNative(hWnd), msg, wparam, lparam);
  }

  return DefWindowProcW(hWnd, msg, wparam, lparam);
}

WWindowWin::~WWindowWin()
{
  DestroyWindow();
}

WResult WWindowWin::InitializeWindow()
{
  W_LOG_BLOCK("WWindowWin::Initialize", m_CreationDescription.m_Title.GetData());

  if (m_bInitialized)
  {
    DestroyWindow();
  }

  W_ASSERT_RELEASE(m_CreationDescription.m_Resolution.HasNonZeroArea(), "The client area size can't be zero sized!");

  // Initialize window class
  WNDCLASSEXW windowClass = {};
  windowClass.cbSize = sizeof(WNDCLASSEXW);
  windowClass.style = CS_HREDRAW | CS_VREDRAW;
  windowClass.hInstance = GetModuleHandleW(nullptr);
  windowClass.hIcon = LoadIcon(GetModuleHandleW(nullptr), MAKEINTRESOURCE(101)); /// \todo Expose icon functionality somehow (101 == IDI_ICON1, see resource.h)
  windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
  windowClass.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
  windowClass.lpszClassName = L"WWin32Window";
  windowClass.lpfnWndProc = WWindowsMessageFuncTrampoline;

  if (!RegisterClassExW(&windowClass)) /// \todo test & support for multiple windows
  {
    DWORD error = GetLastError();

    if (error != ERROR_CLASS_ALREADY_EXISTS)
    {
      WLog::Error("Failed to create WWindowWin window class! (error code '{0}')", WArgErrorCode(error));
      return W_FAILURE;
    }
  }

  // setup fullscreen mode
  if (m_CreationDescription.m_WindowMode == WWindowMode::FullscreenFixedResolution)
  {
    WLog::Dev("Changing display resolution for fullscreen mode to {0}*{1}", m_CreationDescription.m_Resolution.width, m_CreationDescription.m_Resolution.height);

    DEVMODEW dmScreenSettings = {};
    dmScreenSettings.dmSize = sizeof(DEVMODEW);
    dmScreenSettings.dmPelsWidth = m_CreationDescription.m_Resolution.width;
    dmScreenSettings.dmPelsHeight = m_CreationDescription.m_Resolution.height;
    dmScreenSettings.dmBitsPerPel = 32;
    dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

    if (ChangeDisplaySettingsW(&dmScreenSettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
    {
      m_CreationDescription.m_WindowMode = WWindowMode::FullscreenBorderlessNativeResolution;
      W_SUCCEED_OR_RETURN(m_CreationDescription.AdjustWindowSizeAndPosition());

      WLog::Error("Failed to change display resolution for fullscreen window. Falling back to borderless window.");
    }
  }


  // setup window style
  DWORD dwExStyle = WS_EX_APPWINDOW;
  DWORD dwWindowStyle = WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

  if (m_CreationDescription.m_bSetForegroundOnInit && !WSystemInformation::IsDebuggerAttached())
  {
    // use WS_EX_TOPMOST to force that the window shows up on top
    // this is the only thing that seems to be working reliably
    // but to prevent the window from staying on top, we need to remove this flag later again (see SetWindowPos)
    dwExStyle |= WS_EX_TOPMOST;
  }

  if (m_CreationDescription.m_WindowMode == WWindowMode::WindowFixedResolution || m_CreationDescription.m_WindowMode == WWindowMode::WindowResizable)
  {
    WLog::Dev("Window is not fullscreen.");
    dwWindowStyle |= WS_OVERLAPPED | WS_BORDER | WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU;
  }
  else
  {
    WLog::Dev("Window is fullscreen.");
    dwWindowStyle |= WS_POPUP;
  }

  if (m_CreationDescription.m_WindowMode == WWindowMode::WindowResizable)
  {
    WLog::Dev("Window is resizable.");
    dwWindowStyle |= WS_MAXIMIZEBOX | WS_THICKFRAME;
  }


  // Create rectangle for window
  RECT Rect = {0, 0, (LONG)m_CreationDescription.m_Resolution.width, (LONG)m_CreationDescription.m_Resolution.height};

  // Account for left or top placed task bars
  if (m_CreationDescription.m_WindowMode == WWindowMode::WindowFixedResolution || m_CreationDescription.m_WindowMode == WWindowMode::WindowResizable)
  {
    // Adjust for borders and bars etc.
    AdjustWindowRectEx(&Rect, dwWindowStyle, FALSE, dwExStyle);

    // top left position now may be negative (due to AdjustWindowRectEx)
    // move
    Rect.right -= Rect.left;
    Rect.bottom -= Rect.top;
    // apply user translation
    Rect.left = m_CreationDescription.m_Position.x;
    Rect.top = m_CreationDescription.m_Position.y;
    Rect.right += m_CreationDescription.m_Position.x;
    Rect.bottom += m_CreationDescription.m_Position.y;

    // move into work area
    RECT RectWorkArea = {0};
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &RectWorkArea, 0);

    Rect.left += RectWorkArea.left;
    Rect.right += RectWorkArea.left;
    Rect.top += RectWorkArea.top;
    Rect.bottom += RectWorkArea.top;
  }

  const int iWidth = Rect.right - Rect.left;
  const int iHeight = Rect.bottom - Rect.top;

  WLog::Info("Window Dimensions: {0}*{1} at left/top origin ({2}, {3}).", iWidth, iHeight, m_CreationDescription.m_Position.x, m_CreationDescription.m_Position.y);


  // create window
  WStringWChar sTitleWChar(m_CreationDescription.m_Title.GetData());
  const wchar_t* sTitleWCharRaw = sTitleWChar.GetData();
  m_hWindowHandle = WMinWindows::FromNative(CreateWindowExW(dwExStyle, windowClass.lpszClassName, sTitleWCharRaw, dwWindowStyle, m_CreationDescription.m_Position.x, m_CreationDescription.m_Position.y, iWidth, iHeight, nullptr, nullptr, windowClass.hInstance, nullptr));

  if (m_hWindowHandle == INVALID_HANDLE_VALUE)
  {
    WLog::Error("Failed to create window.");
    return W_FAILURE;
  }

  auto windowHandle = WMinWindows::ToNative(m_hWindowHandle);

  // safe window pointer for lookup in WWindowsMessageFuncTrampoline
  SetWindowLongPtrW(windowHandle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

  // show window and activate if required
  ShowWindow(windowHandle, m_CreationDescription.m_bSetForegroundOnInit ? SW_SHOWDEFAULT : SW_SHOWNOACTIVATE);
  if (m_CreationDescription.m_bSetForegroundOnInit)
  {
    SetActiveWindow(windowHandle);
    SetFocus(windowHandle);
    SetForegroundWindow(windowHandle);
  }

  RECT r;
  GetClientRect(windowHandle, &r);

  // Force size change to the desired size if CreateWindowExW 'fixed' the size to fit into your current monitor.
  if (m_CreationDescription.m_WindowMode == WWindowMode::WindowFixedResolution &&
      (m_CreationDescription.m_Resolution.width != WUInt32(r.right - r.left) ||
        m_CreationDescription.m_Resolution.height != WUInt32(r.bottom - r.top)))
  {
    ::SetWindowPos(windowHandle, HWND_NOTOPMOST, 0, 0, iWidth, iHeight, SWP_NOSENDCHANGING | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOZORDER);
    GetClientRect(windowHandle, &r);
  }

  m_CreationDescription.m_Resolution.width = r.right - r.left;
  m_CreationDescription.m_Resolution.height = r.bottom - r.top;



  m_bInitialized = true;
  WLog::Success("Created window successfully. Resolution is {0}*{1}", GetClientAreaSize().width, GetClientAreaSize().height);

  auto pInput = W_DEFAULT_NEW(WInputDeviceMouseKeyboard_Win, WMinWindows::FromNative(windowHandle));
  pInput->SetClipMouseCursor(m_CreationDescription.m_bClipMouseCursor ? WMouseCursorClipMode::ClipToWindowImmediate : WMouseCursorClipMode::NoClip);
  pInput->SetShowMouseCursor(m_CreationDescription.m_bShowMouseCursor);

  m_pInputDevice = std::move(pInput);

  return W_SUCCESS;
}

void WWindowWin::DestroyWindow()
{
  if (!m_bInitialized)
    return;

  if (auto pInput = WDynamicCast<WInputDeviceMouseKeyboard_Win*>(GetInputDevice()))
  {
    pInput->SetClipMouseCursor(WMouseCursorClipMode::NoClip);
  }

  W_LOG_BLOCK("WWindowWin::Destroy");

  m_pInputDevice = nullptr;

  if (m_CreationDescription.m_WindowMode == WWindowMode::FullscreenFixedResolution)
    ChangeDisplaySettingsW(nullptr, 0);

  HWND hWindow = WMinWindows::ToNative(GetNativeWindowHandle());
  // the following line of code is a work around, because 'LONG_PTR pNull = reinterpret_cast<LONG_PTR>(nullptr)' crashes the VS 2010 32 Bit
  // compiler :-(
  LONG_PTR pNull = 0;
  // Set the window ptr to null before calling DestroyWindow as it might trigger callbacks and we are potentially already in the destructor, making any virtual function call unsafe.
  SetWindowLongPtrW(hWindow, GWLP_USERDATA, pNull);

  if (!::DestroyWindow(hWindow))
  {
    WLog::SeriousWarning("DestroyWindow failed.");
  }

  // actually nobody cares about this, all Window Classes are cleared when the application closes
  // in the mean time, having multiple windows will just result in errors when one is closed,
  // as the Window Class must not be in use anymore when one calls UnregisterClassW
  // if (!UnregisterClassW(L"WWin32Window", GetModuleHandleW(nullptr)))
  //{
  //  WLog::SeriousWarning("UnregisterClassW failed.");
  //  Res = W_FAILURE;
  //}

  m_bInitialized = false;
  m_hWindowHandle = INVALID_WINDOW_HANDLE_VALUE;

  WLog::Success("Window destroyed.");
}

WResult WWindowWin::Resize(const WSizeU32& newWindowSize)
{
  auto windowHandle = WMinWindows::ToNative(m_hWindowHandle);
  BOOL res = ::SetWindowPos(windowHandle, HWND_NOTOPMOST, 0, 0, newWindowSize.width, newWindowSize.height, SWP_NOSENDCHANGING | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOZORDER);
  return res != FALSE ? W_SUCCESS : W_FAILURE;
}

void WWindowWin::ProcessWindowMessages()
{
  if (!m_bInitialized)
    return;

  MSG msg = {0};
  while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
  {
    if (msg.message == WM_QUIT)
    {
      DestroyWindow();
      return;
    }

    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }

  if (m_CreationDescription.m_bSetForegroundOnInit)
  {
    // remove the WS_EX_TOPMOST flag again
    m_CreationDescription.m_bSetForegroundOnInit = false;
    HWND hWindow = WMinWindows::ToNative(GetNativeWindowHandle());
    SetWindowPos(hWindow, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
  }
}

WWindowHandle WWindowWin::GetNativeWindowHandle() const
{
  return m_hWindowHandle;
}

#endif
