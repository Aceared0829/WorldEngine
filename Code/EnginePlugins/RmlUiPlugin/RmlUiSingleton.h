#pragma once

#include <RmlUiPlugin/RmlUiPluginDLL.h>

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class WRmlUiContext;
struct WMsgExtractRenderData;

using WRmlUiResourceHandle = WTypedResourceHandle<class WRmlUiResource>;

/// The RML configuration to be used on a specific platform
struct W_RMLUIPLUGIN_DLL WRmlUiConfiguration
{
  WDynamicArray<WString> m_Fonts;

  static constexpr const WStringView s_sConfigFile = ":project/RuntimeConfigs/RmlUiConfig.ddl"_wsv;

  WResult Save(WStringView sFile = s_sConfigFile) const;
  WResult Load(WStringView sFile = s_sConfigFile);

  bool operator==(const WRmlUiConfiguration& rhs) const;
  bool operator!=(const WRmlUiConfiguration& rhs) const { return !operator==(rhs); }
};

class W_RMLUIPLUGIN_DLL WRmlUi
{
  W_DECLARE_SINGLETON(WRmlUi);

public:
  WRmlUi();
  ~WRmlUi();

  WRmlUiContext* CreateContext(const char* szName, const WVec2U32& vInitialSize);
  void DeleteContext(WRmlUiContext* pContext);

  bool AnyContextWantsInput();

  WResult LoadDocumentFromResource(WRmlUiContext& ref_context, const WRmlUiResourceHandle& hResource);
  WResult LoadDocumentFromString(WRmlUiContext& ref_context, const WStringView& sContent);

  void UnloadDocument(WRmlUiContext& ref_context);

  void ClearCaches();

  void ExtractContext(WRmlUiContext& ref_context, WGALTextureHandle hTexture);

  WMutex& GetContextMutex();

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  void DebugContext(WRmlUiContext* pContext);
#endif

private:
  struct Data;
  WUniquePtr<Data> m_pData;
};
