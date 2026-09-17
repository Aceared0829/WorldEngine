
WEditorProcessViewWindow::~WEditorProcessViewWindow()
{
  WGALDevice::GetDefaultDevice()->WaitIdle();

  W_ASSERT_DEV(m_iReferenceCount == 0, "The window is still being referenced, probably by a swapchain. Make sure to destroy all swapchains and call WGALDevice::WaitIdle before destroying a window.");
}

WResult WEditorProcessViewWindow::UpdateWindow(WWindowHandle hParentWindow, WUInt16 uiWidth, WUInt16 uiHeight)
{
  m_hWnd = hParentWindow;
  m_uiWidth = uiWidth;
  m_uiHeight = uiHeight;

  return W_SUCCESS;
}
