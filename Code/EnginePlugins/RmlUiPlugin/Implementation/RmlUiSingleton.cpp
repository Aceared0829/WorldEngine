#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <Foundation/Configuration/CVar.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RmlUiPlugin/Implementation/EventListener.h>
#include <RmlUiPlugin/Implementation/FileInterface.h>
#include <RmlUiPlugin/Implementation/RenderInterface.h>
#include <RmlUiPlugin/Implementation/SystemInterface.h>
#include <RmlUiPlugin/RmlUiContext.h>
#include <RmlUiPlugin/RmlUiSingleton.h>

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT) && W_ENABLED(W_PLATFORM_WINDOWS)
#  include <Foundation/Platform/Win/Utils/IncludeWindows.h>
#  include <RmlUi/Include/RmlUi/Debugger/DebuggerFunctionTable.h>

static Rml_Debugger_Functions s_DebuggerFunctions;

void FillDebuggerFunctionTable()
{
  if (s_DebuggerFunctions.m_InitFunc != nullptr)
    return;

  auto hModule = LoadLibraryW(L"RmlDebugger.dll");
  if (hModule == nullptr)
  {
    WLog::Error("Could not load RmlDebugger.dll");
    return;
  }

  auto func = (GetFunctionsFunc)GetProcAddress(hModule, "Rml_Debugger_GetFunctions");
  if (func == nullptr)
  {
    WLog::Error("Could not find Rml_Debugger_GetFunctions in RmlDebugger.dll");
    return;
  }

  func(&s_DebuggerFunctions);
};

#endif


WResult WRmlUiConfiguration::Save(WStringView sFile) const
{
  W_LOG_BLOCK("WRmlUiConfiguration::Save()");

  WFileWriter file;
  if (file.Open(sFile).Failed())
    return W_FAILURE;

  WOpenDdlWriter writer;
  writer.SetOutputStream(&file);
  writer.SetCompactMode(false);
  writer.SetPrimitiveTypeStringMode(WOpenDdlWriter::TypeStringMode::Compliant);

  writer.BeginObject("Fonts");
  for (auto& font : m_Fonts)
  {
    WOpenDdlUtils::StoreString(writer, font);
  }
  writer.EndObject();

  return W_SUCCESS;
}

WResult WRmlUiConfiguration::Load(WStringView sFile)
{
  W_LOG_BLOCK("WRmlUiConfiguration::Load()");

  m_Fonts.Clear();

  WFileReader file;
  if (file.Open(sFile).Failed())
    return W_FAILURE;

  WOpenDdlReader reader;
  if (reader.ParseDocument(file, 0, WLog::GetThreadLocalLogSystem()).Failed())
  {
    WLog::Error("Failed to parse RmlUi config file '{0}'", sFile);
    return W_FAILURE;
  }

  const WOpenDdlReaderElement* pTree = reader.GetRootElement();

  for (const WOpenDdlReaderElement* pChild = pTree->GetFirstChild(); pChild != nullptr; pChild = pChild->GetSibling())
  {
    if (pChild->IsCustomType("Fonts"))
    {
      for (const WOpenDdlReaderElement* pFont = pChild->GetFirstChild(); pFont != nullptr; pFont = pFont->GetSibling())
      {
        m_Fonts.PushBack(pFont->GetPrimitivesString()[0]);
      }
    }
  }

  return W_SUCCESS;
}

bool WRmlUiConfiguration::operator==(const WRmlUiConfiguration& rhs) const
{
  return m_Fonts == rhs.m_Fonts;
}

//////////////////////////////////////////////////////////////////////////

W_IMPLEMENT_SINGLETON(WRmlUi);

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
WCVarString cvar_RmlUiDebugContext("RmlUi.DebugContext", "", WCVarFlags::Default, "Sets the name of the context that should be debugged");

bool ShouldDebugContext(const WRmlUiContext& context)
{
  if (cvar_RmlUiDebugContext.GetValue().IsEmpty())
    return false;

  WStringView sContextName = WRmlUiConversionUtils::ToStringView(context.GetName());
  return sContextName.FindSubString_NoCase(cvar_RmlUiDebugContext.GetValue()) != nullptr;
}
#endif

struct WRmlUi::Data
{
  WMutex m_ExtractionMutex;
  WRmlUiInternal::RenderInterface m_RenderInterface;

  WRmlUiInternal::FileInterface m_FileInterface;
  WRmlUiInternal::SystemInterface m_SystemInterface;

  WRmlUiInternal::ContextInstancer m_ContextInstancer;
  WRmlUiInternal::EventListenerInstancer m_EventListenerInstancer;

  WMutex m_ContextsMutex;
  WDynamicArray<WRmlUiContext*> m_Contexts;

  WUInt64 m_uiLastClearedCacheFrame = 0;

  WRmlUiConfiguration m_Config;

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  bool m_bDebuggerInitialized = false;
  WEventSubscriptionID m_DebugCVarEventHandler;
#endif
};

WRmlUi::WRmlUi()
  : m_SingletonRegistrar(this)
{
  m_pData = W_DEFAULT_NEW(Data);

  Rml::SetRenderInterface(&m_pData->m_RenderInterface);
  Rml::SetFileInterface(&m_pData->m_FileInterface);
  Rml::SetSystemInterface(&m_pData->m_SystemInterface);

  Rml::Initialise();

  Rml::Factory::RegisterContextInstancer(&m_pData->m_ContextInstancer);
  Rml::Factory::RegisterEventListenerInstancer(&m_pData->m_EventListenerInstancer);

  if (m_pData->m_Config.Load().Failed())
  {
    WLog::Warning("No valid RmlUi configuration file available in '{}'.", WRmlUiConfiguration::s_sConfigFile);
    return;
  }

  for (auto& font : m_pData->m_Config.m_Fonts)
  {
    // Treat last font as fall back
    bool bIsFallbackFont = (font == m_pData->m_Config.m_Fonts.PeekBack());

    if (Rml::LoadFontFace(font.GetData(), bIsFallbackFont) == false)
    {
      WLog::Warning("Failed to load font face '{0}'.", font);
    }
  }

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  m_pData->m_DebugCVarEventHandler = cvar_RmlUiDebugContext.m_CVarEvents.AddEventHandler(
    [this](const WCVarEvent& e)
    {
      if (e.m_EventType != WCVarEvent::ValueChanged)
        return;

      WStringView sContextName = cvar_RmlUiDebugContext.GetValue();
      if (sContextName.IsEmpty())
      {
        DebugContext(nullptr);
        return;
      }

      W_LOCK(m_pData->m_ContextsMutex);

      for (auto pContext : m_pData->m_Contexts)
      {
        if (ShouldDebugContext(*pContext))
        {
          DebugContext(pContext);
          return;
        }
      }

      DebugContext(nullptr);
    });
#endif
}

WRmlUi::~WRmlUi()
{
#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  cvar_RmlUiDebugContext.m_CVarEvents.RemoveEventHandler(m_pData->m_DebugCVarEventHandler);
#endif

  Rml::Shutdown();
}

WRmlUiContext* WRmlUi::CreateContext(const char* szName, const WVec2U32& vInitialSize)
{
  W_LOCK(m_pData->m_ContextsMutex);

  WRmlUiContext* pContext = static_cast<WRmlUiContext*>(Rml::CreateContext(szName, Rml::Vector2i(vInitialSize.x, vInitialSize.y)));
  W_ASSERT_DEV(pContext != nullptr, "RML UI context creation failed");

  m_pData->m_Contexts.PushBack(pContext);

  return pContext;
}

void WRmlUi::DeleteContext(WRmlUiContext* pContext)
{
  W_LOCK(m_pData->m_ContextsMutex);

  m_pData->m_Contexts.RemoveAndCopy(pContext);

  Rml::RemoveContext(pContext->GetName());
}

bool WRmlUi::AnyContextWantsInput()
{
  W_LOCK(m_pData->m_ContextsMutex);

  for (auto pContext : m_pData->m_Contexts)
  {
    if (pContext->WantsInput())
      return true;
  }

  return false;
}

WResult WRmlUi::LoadDocumentFromResource(WRmlUiContext& ref_context, const WRmlUiResourceHandle& hResource)
{
  UnloadDocument(ref_context);

  if (hResource.IsValid())
  {
    WResourceLock<WRmlUiResource> pResource(hResource, WResourceAcquireMode::BlockTillLoaded);
    if (pResource.GetAcquireResult() == WResourceAcquireResult::Final)
    {
      // RmlUi is not thread safe, so we need to make that we only load/unload one document at a time.
      W_LOCK(m_pData->m_ContextsMutex);

      ref_context.LoadDocument(pResource->GetRmlFile().GetData());
    }
  }

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  if (ref_context.HasDocument() && ShouldDebugContext(ref_context))
  {
    DebugContext(&ref_context);
  }
#endif

  return ref_context.HasDocument() ? W_SUCCESS : W_FAILURE;
}

WResult WRmlUi::LoadDocumentFromString(WRmlUiContext& ref_context, const WStringView& sContent)
{
  UnloadDocument(ref_context);

  if (!sContent.IsEmpty())
  {
    Rml::String sRmlContent = Rml::String(sContent.GetStartPointer(), sContent.GetElementCount());

    // RmlUi is not thread safe, so we need to make that we only load/unload one document at a time.
    W_LOCK(m_pData->m_ContextsMutex);

    ref_context.LoadDocumentFromMemory(sRmlContent);
  }

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  if (ref_context.HasDocument() && ShouldDebugContext(ref_context))
  {
    DebugContext(&ref_context);
  }
#endif

  return ref_context.HasDocument() ? W_SUCCESS : W_FAILURE;
}

void WRmlUi::UnloadDocument(WRmlUiContext& ref_context)
{
  if (ref_context.HasDocument())
  {
    // RmlUi is not thread safe, so we need to make that we only load/unload one document at a time.
    W_LOCK(m_pData->m_ContextsMutex);

    static_cast<Rml::Context&>(ref_context).UnloadDocument(ref_context.GetDocument(0));
  }
}

void WRmlUi::ClearCaches()
{
  WUInt64 uiCurrentFrame = WRenderWorld::GetFrameCounter();
  if (uiCurrentFrame == m_pData->m_uiLastClearedCacheFrame)
    return;

  m_pData->m_uiLastClearedCacheFrame = uiCurrentFrame;

  W_LOCK(m_pData->m_ContextsMutex);
  Rml::Factory::ClearStyleSheetCache();
  Rml::Factory::ClearTemplateCache();
}

void WRmlUi::ExtractContext(WRmlUiContext& ref_context, WGALTextureHandle hTexture)
{
  if (ref_context.HasDocument() == false)
    return;

  // Unfortunately we need to hold a lock for the whole extraction of a context since RmlUi is not thread safe.
  W_LOCK(m_pData->m_ExtractionMutex);

  ref_context.ExtractRenderData(m_pData->m_RenderInterface, hTexture);
}

WMutex& WRmlUi::GetContextMutex()
{
  return m_pData->m_ContextsMutex;
}

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
void WRmlUi::DebugContext(WRmlUiContext* pContext)
{
#  if W_ENABLED(W_PLATFORM_WINDOWS)
  FillDebuggerFunctionTable();

  if (s_DebuggerFunctions.m_InitFunc == nullptr)
    return;

  W_LOCK(m_pData->m_ContextsMutex);

  if (m_pData->m_bDebuggerInitialized)
  {
    s_DebuggerFunctions.m_ShutdownFunc();
    m_pData->m_bDebuggerInitialized = false;
  }

  if (pContext != nullptr)
  {
    s_DebuggerFunctions.m_InitFunc(pContext);
    s_DebuggerFunctions.m_SetVisibleFunc(true);
    m_pData->m_bDebuggerInitialized = true;
  }
#  else
  WLog::Error("RmlUi debugger is only available on Windows.");
#  endif
}
#endif


W_STATICLINK_FILE(RmlUiPlugin, RmlUiPlugin_Implementation_RmlUiSingleton);
