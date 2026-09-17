#pragma once

#include <Core/Interfaces/FrameCaptureInterface.h>
#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Configuration/Singleton.h>
#include <RenderDocPlugin/RenderDocPluginDLL.h>

struct RENDERDOC_API_1_4_1;

/// RenderDoc implementation of the WFrameCaptureInterface interface
///
/// Adds support for capturing frames through RenderDoc.
/// When the plugin gets loaded, an WRenderDoc instance is created and initialized.
/// It tries to find a RenderDoc DLL dynamically, so for initialization to succeed,
/// the DLL has to be available in some search directory (e.g. binary folder or PATH).
/// If an outdated RenderDoc DLL is found, initialization will fail and the plugin will be deactivated.
///
/// For interface documentation see \ref WFrameCaptureInterface
class W_RENDERDOCPLUGIN_DLL WRenderDoc : public WFrameCaptureInterface
{
  W_DECLARE_SINGLETON_OF_INTERFACE(WRenderDoc, WFrameCaptureInterface);

public:
  WRenderDoc();
  virtual ~WRenderDoc();

  virtual bool IsInitialized() const override;
  virtual void SetAbsCaptureFilePathTemplate(WStringView sFilePathTemplate) override;
  virtual WStringView GetAbsCaptureFilePathTemplate() const override;
  virtual void StartFrameCapture(WWindowHandle hWnd) override;
  virtual bool IsFrameCapturing() const override;
  virtual void EndFrameCaptureAndWriteOutput(WWindowHandle hWnd) override;
  virtual void EndFrameCaptureAndDiscardResult(WWindowHandle hWnd) override;
  virtual WResult GetLastAbsCaptureFileName(WStringBuilder& out_sFileName) const override;

private:
  RENDERDOC_API_1_4_1* m_pRenderDocAPI = nullptr;
  WMinWindows::HMODULE m_pHandleToFree = nullptr;
};
