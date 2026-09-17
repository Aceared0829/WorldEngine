#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/GUI/ExposedParametersTypeRegistry.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/GUI/ExposedParameters.h>
#include <Foundation/Serialization/ReflectionSerializer.h>

W_IMPLEMENT_SINGLETON(WExposedParametersTypeRegistry);

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(EditorFramework, ExposedParametersTypeRegistry)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "ReflectedTypeManager", "AssetCurator"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    W_DEFAULT_NEW(WExposedParametersTypeRegistry);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    WExposedParametersTypeRegistry* pDummy = WExposedParametersTypeRegistry::GetSingleton();
    W_DEFAULT_DELETE(pDummy);
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
  }

W_END_SUBSYSTEM_DECLARATION;
// clang-format on

WExposedParametersTypeRegistry::WExposedParametersTypeRegistry()
  : m_SingletonRegistrar(this)
{
  WReflectedTypeDescriptor desc;
  desc.m_sTypeName = "WExposedParametersTypeBase";
  desc.m_sPluginName = "ExposedParametersTypes";
  desc.m_sParentTypeName = WGetStaticRTTI<WReflectedClass>()->GetTypeName();
  desc.m_Flags = WTypeFlags::Abstract | WTypeFlags::Class;
  desc.m_uiTypeVersion = 0;

  m_pBaseType = WPhantomRttiManager::RegisterType(desc);

  WAssetCurator::GetSingleton()->m_Events.AddEventHandler(WMakeDelegate(&WExposedParametersTypeRegistry::AssetCuratorEventHandler, this));
  WPhantomRttiManager::s_Events.AddEventHandler(WMakeDelegate(&WExposedParametersTypeRegistry::PhantomTypeRegistryEventHandler, this));
}


WExposedParametersTypeRegistry::~WExposedParametersTypeRegistry()
{
  WAssetCurator::GetSingleton()->m_Events.RemoveEventHandler(WMakeDelegate(&WExposedParametersTypeRegistry::AssetCuratorEventHandler, this));
  WPhantomRttiManager::s_Events.RemoveEventHandler(WMakeDelegate(&WExposedParametersTypeRegistry::PhantomTypeRegistryEventHandler, this));
}

const WRTTI* WExposedParametersTypeRegistry::GetExposedParametersType(const char* szResource)
{
  if (WStringUtils::IsNullOrEmpty(szResource))
    return nullptr;

  const auto asset = WAssetCurator::GetSingleton()->FindSubAsset(szResource);
  if (!asset)
    return nullptr;

  auto params = asset->m_pAssetInfo->m_Info->GetMetaInfo<WExposedParameters>();
  if (!params)
    return nullptr;

  auto it = m_ShaderTypes.Find(asset->m_Data.m_Guid);
  if (it.IsValid())
  {
    if (!it.Value().m_bUpToDate)
    {
      UpdateExposedParametersType(it.Value(), *params);
    }
  }
  else
  {
    it = m_ShaderTypes.Insert(asset->m_Data.m_Guid, ParamData());
    it.Value().m_SubAssetGuid = asset->m_Data.m_Guid;
    UpdateExposedParametersType(it.Value(), *params);
  }

  return it.Value().m_pType;
}

void WExposedParametersTypeRegistry::UpdateExposedParametersType(ParamData& data, const WExposedParameters& params)
{
  WStringBuilder name;
  name.SetFormat("WExposedParameters_{0}", data.m_SubAssetGuid);
  W_LOG_BLOCK("Updating Type", name.GetData());
  WReflectedTypeDescriptor desc;
  desc.m_sTypeName = name;
  desc.m_sPluginName = "ExposedParametersTypes";
  desc.m_sParentTypeName = m_pBaseType->GetTypeName();
  desc.m_Flags = WTypeFlags::Class;
  desc.m_uiTypeVersion = 2;

  for (const auto* parameter : params.m_Parameters)
  {
    const WRTTI* pType = WReflectionUtils::GetTypeFromVariant(parameter->m_DefaultValue);
    if (!parameter->m_sType.IsEmpty())
    {
      if (const WRTTI* pType2 = WRTTI::FindTypeByName(parameter->m_sType))
        pType = pType2;
    }
    if (pType == nullptr)
    {
      WLog::Warning("The exposed parameter '{}' on type '{}' does not have a type defined as is skipped.", parameter->m_sName, name);
      continue;
    }
    WBitflags<WPropertyFlags> flags;
    if (pType->IsDerivedFrom<WEnumBase>())
      flags |= WPropertyFlags::IsEnum;
    if (pType->IsDerivedFrom<WBitflagsBase>())
      flags |= WPropertyFlags::Bitflags;
    if (WReflectionUtils::IsBasicType(pType))
      flags |= WPropertyFlags::StandardType;
    else
      flags |= WPropertyFlags::Class;

    WReflectedPropertyDescriptor propDesc(parameter->m_Category, parameter->m_sName, pType->GetTypeName(), flags);
    for (auto attrib : parameter->m_Attributes)
    {
      propDesc.m_Attributes.PushBack(WReflectionSerializer::Clone(attrib));
    }
    if (parameter->m_DefaultValue.IsValid())
    {
      propDesc.m_Attributes.PushBack(W_DEFAULT_NEW(WDefaultValueAttribute, parameter->m_DefaultValue));
    }
    desc.m_Properties.PushBack(propDesc);
  }

  // Register and return the phantom type. If the type already exists this will update the type
  // and patch any existing instances of it so they should show up in the prop grid right away.
  {
    // This fkt is called by the property grid, but calling RegisterType will update the property grid
    // and we will recurse into this. So we listen for the WPhantomRttiManager events to fill out
    // the data.m_pType in it to make sure recursion into GetExposedParametersType does not return a nullptr.
    m_pAboutToBeRegistered = &data;
    data.m_bUpToDate = true;
    data.m_pType = WPhantomRttiManager::RegisterType(desc);
    m_pAboutToBeRegistered = nullptr;
  }
}

void WExposedParametersTypeRegistry::AssetCuratorEventHandler(const WAssetCuratorEvent& e)
{
  switch (e.m_Type)
  {
    case WAssetCuratorEvent::Type::AssetRemoved:
    {
      // Ignore for now, doesn't hurt. Removing types is more hassle than it is worth.
      if (auto* data = m_ShaderTypes.GetValue(e.m_AssetGuid))
      {
        data->m_bUpToDate = false;
      }
    }
    break;
    case WAssetCuratorEvent::Type::AssetListReset:
    {
      for (auto it = m_ShaderTypes.GetIterator(); it.IsValid(); ++it)
      {
        it.Value().m_bUpToDate = false;
      }
    }
    break;
    case WAssetCuratorEvent::Type::AssetUpdated:
    {
      if (auto* data = m_ShaderTypes.GetValue(e.m_AssetGuid))
      {
        data->m_bUpToDate = false;
        if (auto params = e.m_pInfo->m_pAssetInfo->m_Info->GetMetaInfo<WExposedParameters>())
          UpdateExposedParametersType(*data, *params);
      }
    }
    break;
    default:
      break;
  }
}

void WExposedParametersTypeRegistry::PhantomTypeRegistryEventHandler(const WPhantomRttiManagerEvent& e)
{
  if (e.m_Type == WPhantomRttiManagerEvent::Type::TypeAdded || e.m_Type == WPhantomRttiManagerEvent::Type::TypeChanged)
  {
    if (e.m_pChangedType->GetParentType() == m_pBaseType && m_pAboutToBeRegistered)
    {
      // We listen for the WPhantomRttiManager events to fill out the m_pType pointer. This is needed as otherwise
      // Recursion into GetExposedParametersType would return a nullptr.
      m_pAboutToBeRegistered->m_pType = e.m_pChangedType;
    }
  }
}
