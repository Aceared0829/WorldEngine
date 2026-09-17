#pragma once

class W_CORE_DLL WWindowNoImpl : public WWindowPlatformShared
{
public:
  ~WWindowNoImpl();

  virtual WResult InitializeWindow() override;
  virtual void DestroyWindow() override;
  virtual WResult Resize(const WSizeU32& newWindowSize) override;
  virtual void ProcessWindowMessages() override;
  virtual WWindowHandle GetNativeWindowHandle() const override;
};


// can't use a 'using' here, because that can't be forward declared
class W_CORE_DLL WWindow : public WWindowNoImpl
{
};
