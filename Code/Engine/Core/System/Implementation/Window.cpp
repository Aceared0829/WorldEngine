#include <Core/CorePCH.h>

#include <Core/System/Window.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <Foundation/System/Screen.h>

WResult WWindowCreationDesc::AdjustWindowSizeAndPosition()
{
  WTempHybridArray<WScreenInfo, 2> screens;
  if (WScreen::EnumerateScreens(screens).Failed() || screens.IsEmpty())
    return W_FAILURE;

  WInt32 iShowOnMonitor = m_iMonitor;

  if (iShowOnMonitor >= (WInt32)screens.GetCount())
    iShowOnMonitor = -1;

  const WScreenInfo* pScreen = nullptr;

  // this means 'pick the primary screen'
  if (iShowOnMonitor < 0)
  {
    pScreen = &screens[0];

    for (WUInt32 i = 0; i < screens.GetCount(); ++i)
    {
      if (screens[i].m_bIsPrimary)
      {
        pScreen = &screens[i];
        break;
      }
    }
  }
  else
  {
    pScreen = &screens[iShowOnMonitor];
  }

  if (m_WindowMode == WWindowMode::FullscreenBorderlessNativeResolution)
  {
    m_Resolution.width = pScreen->m_iResolutionX;
    m_Resolution.height = pScreen->m_iResolutionY;
  }
  else
  {
    // clamp the resolution to the native resolution ?
    // m_ClientAreaSize.width = WMath::Min<WUInt32>(m_ClientAreaSize.width, pScreen->m_iResolutionX);
    // m_ClientAreaSize.height= WMath::Min<WUInt32>(m_ClientAreaSize.height,pScreen->m_iResolutionY);
  }

  if (m_bCenterWindowOnDisplay)
  {
    m_Position.Set(pScreen->m_iOffsetX + (pScreen->m_iResolutionX - (WInt32)m_Resolution.width) / 2, pScreen->m_iOffsetY + (pScreen->m_iResolutionY - (WInt32)m_Resolution.height) / 2);
  }
  else
  {
    m_Position.Set(pScreen->m_iOffsetX, pScreen->m_iOffsetY);
  }

  return W_SUCCESS;
}

void WWindowCreationDesc::SaveToDDL(WOpenDdlWriter& ref_writer)
{
  ref_writer.BeginObject("WindowDesc");

  WOpenDdlUtils::StoreString(ref_writer, m_Title, "Title");

  switch (m_WindowMode.GetValue())
  {
    case WWindowMode::FullscreenBorderlessNativeResolution:
      WOpenDdlUtils::StoreString(ref_writer, "Borderless", "Mode");
      break;
    case WWindowMode::FullscreenFixedResolution:
      WOpenDdlUtils::StoreString(ref_writer, "Fullscreen", "Mode");
      break;
    case WWindowMode::WindowFixedResolution:
      WOpenDdlUtils::StoreString(ref_writer, "Window", "Mode");
      break;
    case WWindowMode::WindowResizable:
      WOpenDdlUtils::StoreString(ref_writer, "ResizableWindow", "Mode");
      break;
  }

  if (m_iMonitor >= 0)
    WOpenDdlUtils::StoreInt8(ref_writer, m_iMonitor, "Monitor");

  if (m_Position != WVec2I32(0x80000000, 0x80000000))
  {
    WOpenDdlUtils::StoreVec2I(ref_writer, m_Position, "Position");
  }

  WOpenDdlUtils::StoreVec2U(ref_writer, WVec2U32(m_Resolution.width, m_Resolution.height), "Resolution");

  WOpenDdlUtils::StoreBool(ref_writer, m_bClipMouseCursor, "ClipMouseCursor");
  WOpenDdlUtils::StoreBool(ref_writer, m_bShowMouseCursor, "ShowMouseCursor");
  WOpenDdlUtils::StoreBool(ref_writer, m_bSetForegroundOnInit, "SetForegroundOnInit");
  WOpenDdlUtils::StoreBool(ref_writer, m_bCenterWindowOnDisplay, "CenterWindowOnDisplay");

  ref_writer.EndObject();
}


WResult WWindowCreationDesc::SaveToDDL(WStringView sFile)
{
  WFileWriter file;
  W_SUCCEED_OR_RETURN(file.Open(sFile));

  WOpenDdlWriter writer;
  writer.SetOutputStream(&file);

  SaveToDDL(writer);

  return W_SUCCESS;
}

void WWindowCreationDesc::LoadFromDDL(const WOpenDdlReaderElement* pParentElement)
{
  if (const WOpenDdlReaderElement* pDesc = pParentElement->FindChildOfType("WindowDesc"))
  {
    if (const WOpenDdlReaderElement* pTitle = pDesc->FindChildOfType(WOpenDdlPrimitiveType::String, "Title"))
      m_Title = pTitle->GetPrimitivesString()[0];

    if (const WOpenDdlReaderElement* pMode = pDesc->FindChildOfType(WOpenDdlPrimitiveType::String, "Mode"))
    {
      auto mode = pMode->GetPrimitivesString()[0];

      if (mode == "Borderless")
        m_WindowMode = WWindowMode::FullscreenBorderlessNativeResolution;
      else if (mode == "Fullscreen")
        m_WindowMode = WWindowMode::FullscreenFixedResolution;
      else if (mode == "Window")
        m_WindowMode = WWindowMode::WindowFixedResolution;
      else if (mode == "ResizableWindow")
        m_WindowMode = WWindowMode::WindowResizable;
    }

    if (const WOpenDdlReaderElement* pMonitor = pDesc->FindChildOfType(WOpenDdlPrimitiveType::Int8, "Monitor"))
    {
      m_iMonitor = pMonitor->GetPrimitivesInt8()[0];
    }

    if (const WOpenDdlReaderElement* pPosition = pDesc->FindChild("Position"))
    {
      WOpenDdlUtils::ConvertToVec2I(pPosition, m_Position).IgnoreResult();
    }

    if (const WOpenDdlReaderElement* pPosition = pDesc->FindChild("Resolution"))
    {
      WVec2U32 res;
      WOpenDdlUtils::ConvertToVec2U(pPosition, res).IgnoreResult();
      m_Resolution.width = res.x;
      m_Resolution.height = res.y;
    }

    if (const WOpenDdlReaderElement* pClipMouseCursor = pDesc->FindChildOfType(WOpenDdlPrimitiveType::Bool, "ClipMouseCursor"))
      m_bClipMouseCursor = pClipMouseCursor->GetPrimitivesBool()[0];

    if (const WOpenDdlReaderElement* pShowMouseCursor = pDesc->FindChildOfType(WOpenDdlPrimitiveType::Bool, "ShowMouseCursor"))
      m_bShowMouseCursor = pShowMouseCursor->GetPrimitivesBool()[0];

    if (const WOpenDdlReaderElement* pSetForegroundOnInit = pDesc->FindChildOfType(WOpenDdlPrimitiveType::Bool, "SetForegroundOnInit"))
      m_bSetForegroundOnInit = pSetForegroundOnInit->GetPrimitivesBool()[0];

    if (const WOpenDdlReaderElement* pCenterWindowOnDisplay = pDesc->FindChildOfType(WOpenDdlPrimitiveType::Bool, "CenterWindowOnDisplay"))
      m_bCenterWindowOnDisplay = pCenterWindowOnDisplay->GetPrimitivesBool()[0];
  }
}

WResult WWindowCreationDesc::LoadFromDDL(WStringView sFile)
{
  WFileReader file;
  W_SUCCEED_OR_RETURN(file.Open(sFile));

  WOpenDdlReader reader;
  W_SUCCEED_OR_RETURN(reader.ParseDocument(file));

  LoadFromDDL(reader.GetRootElement());

  return W_SUCCESS;
}

WWindowPlatformShared::WWindowPlatformShared() = default;

WWindowPlatformShared::~WWindowPlatformShared()
{
  W_ASSERT_DEV(m_iReferenceCount == 0, "The window is still being referenced, probably by a swapchain. Make sure to destroy all swapchains and call WGALDevice::WaitIdle before destroying a window.");

  WWindowEvent e;
  e.m_Type = WWindowEvent::Type::WindowDestruction;
  e.m_pWindow = this;

  m_WindowEvents.Broadcast(e);
}

void WWindowPlatformShared::OnResize(const WSizeU32& newWindowSize)
{
  m_CreationDescription.m_Resolution = newWindowSize;

  WWindowEvent e;
  e.m_Type = WWindowEvent::Type::SizeChanged;
  e.m_pWindow = this;
  e.m_iPayload1 = newWindowSize.width;
  e.m_iPayload2 = newWindowSize.height;

  m_WindowEvents.Broadcast(e);
}

void WWindowPlatformShared::OnWindowMove(const WInt32 iNewPosX, const WInt32 iNewPosY)
{
  WWindowEvent e;
  e.m_Type = WWindowEvent::Type::PositionChanged;
  e.m_pWindow = this;
  e.m_iPayload1 = iNewPosX;
  e.m_iPayload2 = iNewPosY;

  m_WindowEvents.Broadcast(e);
}

void WWindowPlatformShared::OnFocus(bool bHasFocus)
{
  m_bHasFocus = bHasFocus;

  WWindowEvent e;
  e.m_Type = WWindowEvent::Type::FocusChanged;
  e.m_pWindow = this;
  e.m_iPayload1 = bHasFocus ? 1 : 0;

  m_WindowEvents.Broadcast(e);
}

void WWindowPlatformShared::OnVisibleChange(bool bVisible)
{
  m_bVisible = bVisible;

  WWindowEvent e;
  e.m_Type = WWindowEvent::Type::VisibilityChanged;
  e.m_pWindow = this;
  e.m_iPayload1 = bVisible ? 1 : 0;

  m_WindowEvents.Broadcast(e);
}

void WWindowPlatformShared::OnClickClose()
{
  WWindowEvent e;
  e.m_Type = WWindowEvent::Type::CloseButtonClicked;
  e.m_pWindow = this;

  m_WindowEvents.Broadcast(e);
}
