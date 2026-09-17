
#if W_ENABLED(W_SUPPORTS_GLFW)

#  include <Core/Platform/GLFW/Window_GLFW.h>

#else

class W_CORE_DLL WWindowWin : public WWindowPlatformShared
{
public:
  ~WWindowWin();

  virtual WResult InitializeWindow() override;
  virtual void DestroyWindow() override;
  virtual WResult Resize(const WSizeU32& newWindowSize) override;
  virtual void ProcessWindowMessages() override;
  virtual WWindowHandle GetNativeWindowHandle() const override;

  /// Called on any window message.
  ///
  /// You can use this function for example to dispatch the message to another system.
  ///
  /// \remarks
  ///   Will be called <i>after</i> the On[...] callbacks!
  ///
  /// \see OnResizeMessage
  virtual void OnWindowMessage(WMinWindows::HWND hWnd, WMinWindows::UINT msg, WMinWindows::WPARAM wparam, WMinWindows::LPARAM lparam)
  {
    W_IGNORE_UNUSED(hWnd);
    W_IGNORE_UNUSED(msg);
    W_IGNORE_UNUSED(wparam);
    W_IGNORE_UNUSED(lparam);
  }
};

// can't use a 'using' here, because that can't be forward declared
class W_CORE_DLL WWindow : public WWindowWin
{
};

#endif
