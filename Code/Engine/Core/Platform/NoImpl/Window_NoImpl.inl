#include <Core/System/Window.h>

#include <Core/Platform/NoImpl/Window_NoImpl.h>

WWindowNoImpl::~WWindowNoImpl()
{
}

WResult WWindowNoImpl::InitializeWindow()
{
  W_ASSERT_NOT_IMPLEMENTED;
  return W_FAILURE;
}

void WWindowNoImpl::DestroyWindow()
{
  W_ASSERT_NOT_IMPLEMENTED;
}

WResult WWindowNoImpl::Resize(const WSizeU32& newWindowSize)
{
  W_ASSERT_NOT_IMPLEMENTED;
  return W_FAILURE;
}

void WWindowNoImpl::ProcessWindowMessages()
{
  W_ASSERT_NOT_IMPLEMENTED;
}

WWindowHandle WWindowNoImpl::GetNativeWindowHandle() const
{
  return m_hWindowHandle;
}
