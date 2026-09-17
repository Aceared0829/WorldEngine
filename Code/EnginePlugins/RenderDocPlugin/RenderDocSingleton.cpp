#include <RenderDocPlugin/RenderDocPluginPCH.h>

#include <Foundation/Platform/Win/Utils/IncludeWindows.h>

#include <Foundation/Configuration/CVar.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <RenderDocPlugin/RenderDocSingleton.h>
#include <RenderDocPlugin/ThirdParty/renderdoc_app.h>

W_IMPLEMENT_SINGLETON(WRenderDoc);

static WCommandLineOptionBool opt_NoCaptures("RenderDoc", "-NoCaptures", "Disables RenderDoc capture support.", false);

static WRenderDoc g_RenderDocSingleton;

WRenderDoc::WRenderDoc()
  : m_SingletonRegistrar(this)
{
  if (opt_NoCaptures.GetOptionValue(WCommandLineOption::LogMode::AlwaysIfSpecified))
  {
    WLog::Info("RenderDoc plugin: Initialization suppressed via command-line.");
    return;
  }

  HMODULE dllHandle = GetModuleHandleW(L"renderdoc.dll");
  if (!dllHandle)
  {
    dllHandle = LoadLibraryW(L"renderdoc.dll");
    m_pHandleToFree = WMinWindows::FromNative(dllHandle);
  }

  if (!dllHandle)
  {
    WLog::Info("RenderDoc plugin: 'renderdoc.dll' could not be found. Frame captures aren't possible.");
    return;
  }

  if (pRENDERDOC_GetAPI RenderDoc_GetAPI = (pRENDERDOC_GetAPI)GetProcAddress(dllHandle, "RENDERDOC_GetAPI"))
  {
    void* pApi = nullptr;
    RenderDoc_GetAPI(eRENDERDOC_API_Version_1_4_0, &pApi);
    m_pRenderDocAPI = (RENDERDOC_API_1_4_1*)pApi;
  }

  if (m_pRenderDocAPI)
  {
    m_pRenderDocAPI->SetCaptureKeys(nullptr, 0);
    m_pRenderDocAPI->SetFocusToggleKeys(nullptr, 0);
    m_pRenderDocAPI->MaskOverlayBits(0, 0);
  }
  else
  {
    WLog::Warning("RenderDoc plugin: Unable to retrieve API pointer from DLL. Potentially outdated version. Frame captures aren't possible.");
  }
}

WRenderDoc::~WRenderDoc()
{
  m_pRenderDocAPI = nullptr;

  if (m_pHandleToFree)
  {
    FreeLibrary(WMinWindows::ToNative(m_pHandleToFree));
    m_pHandleToFree = nullptr;
  }
}

bool WRenderDoc::IsInitialized() const
{
  return m_pRenderDocAPI != nullptr;
}

void WRenderDoc::SetAbsCaptureFilePathTemplate(WStringView sFilePathTemplate)
{
  if (m_pRenderDocAPI)
  {
    WStringBuilder tmp;
    m_pRenderDocAPI->SetCaptureFilePathTemplate(sFilePathTemplate.GetData(tmp));
  }
}

WStringView WRenderDoc::GetAbsCaptureFilePathTemplate() const
{
  if (m_pRenderDocAPI)
  {
    return m_pRenderDocAPI->GetCaptureFilePathTemplate();
  }

  return {};
}

void WRenderDoc::StartFrameCapture(WWindowHandle hWnd)
{
  if (m_pRenderDocAPI)
  {
    m_pRenderDocAPI->StartFrameCapture(nullptr, hWnd);
  }
}

bool WRenderDoc::IsFrameCapturing() const
{
  return m_pRenderDocAPI ? m_pRenderDocAPI->IsFrameCapturing() : false;
}

void WRenderDoc::EndFrameCaptureAndWriteOutput(WWindowHandle hWnd)
{
  if (m_pRenderDocAPI)
  {
    m_pRenderDocAPI->EndFrameCapture(nullptr, hWnd);
  }
}

void WRenderDoc::EndFrameCaptureAndDiscardResult(WWindowHandle hWnd)
{
  if (m_pRenderDocAPI)
  {
    m_pRenderDocAPI->DiscardFrameCapture(nullptr, hWnd);
  }
}

WResult WRenderDoc::GetLastAbsCaptureFileName(WStringBuilder& out_sFileName) const
{
  if (m_pRenderDocAPI && m_pRenderDocAPI->GetNumCaptures() > 0)
  {
    WUInt32 uiNumCaptures = m_pRenderDocAPI->GetNumCaptures();
    WUInt32 uiFilePathLength = 0;
    if (m_pRenderDocAPI->GetCapture(uiNumCaptures - 1, nullptr, &uiFilePathLength, nullptr))
    {
      WTempHybridArray<char, 128> filePathBuffer;
      filePathBuffer.SetCount(uiFilePathLength);
      m_pRenderDocAPI->GetCapture(uiNumCaptures - 1, filePathBuffer.GetArrayPtr().GetPtr(), nullptr, nullptr);
      out_sFileName = filePathBuffer.GetArrayPtr().GetPtr();

      return W_SUCCESS;
    }
  }

  return W_FAILURE;
}
