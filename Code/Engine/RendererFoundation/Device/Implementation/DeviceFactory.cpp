#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Device/DeviceFactory.h>

struct CreatorFuncInfo
{
  WGALDeviceFactory::CreatorFunc m_Func;
  WString m_sShaderModel;
  WString m_sShaderCompiler;
};

static WHashTable<WString, CreatorFuncInfo> s_CreatorFuncs;

CreatorFuncInfo* GetCreatorFuncInfo(WStringView sRendererName)
{
  auto pFuncInfo = s_CreatorFuncs.GetValue(sRendererName);
  if (pFuncInfo == nullptr)
  {
    WStringBuilder sPluginName = "WRenderer";
    sPluginName.Append(sRendererName);

    W_VERIFY(WPlugin::LoadPlugin(sPluginName).Succeeded(), "Renderer plugin '{}' not found", sPluginName);

    pFuncInfo = s_CreatorFuncs.GetValue(sRendererName);
    W_ASSERT_DEV(pFuncInfo != nullptr, "Renderer '{}' is not registered", sRendererName);
  }

  return pFuncInfo;
}

WInternal::NewInstance<WGALDevice> WGALDeviceFactory::CreateDevice(WStringView sRendererName, WAllocator* pAllocator, const WGALDeviceCreationDescription& desc)
{
  if (auto pFuncInfo = GetCreatorFuncInfo(sRendererName))
  {
    return pFuncInfo->m_Func(pAllocator, desc);
  }

  return WInternal::NewInstance<WGALDevice>(nullptr, pAllocator);
}

void WGALDeviceFactory::GetShaderModelAndCompiler(WStringView sRendererName, const char*& ref_szShaderModel, const char*& ref_szShaderCompiler)
{
  if (auto pFuncInfo = GetCreatorFuncInfo(sRendererName))
  {
    ref_szShaderModel = pFuncInfo->m_sShaderModel;
    ref_szShaderCompiler = pFuncInfo->m_sShaderCompiler;
  }
}

void WGALDeviceFactory::RegisterCreatorFunc(const char* szRendererName, const CreatorFunc& func, const char* szShaderModel, const char* szShaderCompiler)
{
  CreatorFuncInfo funcInfo;
  funcInfo.m_Func = func;
  funcInfo.m_sShaderModel = szShaderModel;
  funcInfo.m_sShaderCompiler = szShaderCompiler;

  W_VERIFY(s_CreatorFuncs.Insert(szRendererName, funcInfo) == false, "Creator func already registered");
}

void WGALDeviceFactory::UnregisterCreatorFunc(const char* szRendererName)
{
  W_VERIFY(s_CreatorFuncs.Remove(szRendererName), "Creator func not registered");
}
