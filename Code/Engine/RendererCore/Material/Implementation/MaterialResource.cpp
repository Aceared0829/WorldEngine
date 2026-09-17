#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Material/MaterialResource.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <RendererCore/Material/MaterialManager.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Shader/ShaderPermutationResource.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererCore/Textures/TextureCubeResource.h>
#include <RendererCore/Textures/TextureLoader.h>
#include <Texture/Image/Formats/DdsFileFormat.h>

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
#  include <Foundation/IO/CompressedStreamZstd.h>
#endif

namespace
{
  template <typename Source, typename Target>
  void CopyMaterialDesc(const Source& source, Target& ref_target)
  {
    ref_target.Clear();
    ref_target.Reserve(source.GetCount());
    for (const auto& entry : source)
    {
      ref_target.PushBack({entry.Key(), entry.Value()});
    }
  }

  template <typename Property>
  struct SetNameHelper
  {
    W_ALWAYS_INLINE void SetName(Property& ref_prop, const char* szName) { ref_prop.m_Name.Assign(szName); }
    W_ALWAYS_INLINE void SetName(Property& ref_prop, WHashedString sName) { ref_prop.m_Name = sName; }
  };

  template <typename Value, typename Property, typename Name>
  Value GetProperty(WDynamicArray<Property>& ref_properties, Name sName)
  {
    for (WUInt32 i = 0; i < ref_properties.GetCount(); ++i)
    {
      if (ref_properties[i].m_Name == sName)
      {
        return ref_properties[i].m_Value;
      }
    }
    return {};
  }

  template <typename Property, typename Name, typename Value, typename NameLookup>
  bool SetProperty(WDynamicArray<Property>& ref_properties, const Name& sName, const Value& value, const NameLookup& sNameLookup)
  {
    SetNameHelper<Property> setNameHelper;
    WUInt32 uiIndex = WInvalidIndex;
    for (WUInt32 i = 0; i < ref_properties.GetCount(); ++i)
    {
      if (ref_properties[i].m_Name == sNameLookup)
      {
        uiIndex = i;
        break;
      }
    }

    if (value.IsValid())
    {
      if (uiIndex != WInvalidIndex)
      {
        if (ref_properties[uiIndex].m_Value == value)
        {
          return false;
        }

        ref_properties[uiIndex].m_Value = value;
      }
      else
      {
        auto& param = ref_properties.ExpandAndGetRef();
        setNameHelper.SetName(param, sName);
        param.m_Value = value;
      }
    }
    else
    {
      if (uiIndex == WInvalidIndex)
      {
        return false;
      }

      ref_properties.RemoveAtAndSwap(uiIndex);
    }
    return true;
  }
} // namespace

void WMaterialResourceDescriptor::Clear()
{
  m_hBaseMaterial.Invalidate();
  m_sSurface.Clear();
  m_hShader.Invalidate();
  m_PermutationVars.Clear();
  m_Parameters.Clear();
  m_Texture2DBindings.Clear();
  m_TextureCubeBindings.Clear();
  m_RenderDataCategory = WInvalidRenderDataCategory;
}

bool WMaterialResourceDescriptor::operator==(const WMaterialResourceDescriptor& other) const
{
  return m_hBaseMaterial == other.m_hBaseMaterial &&
         m_hShader == other.m_hShader &&
         m_PermutationVars == other.m_PermutationVars &&
         m_Parameters == other.m_Parameters &&
         m_Texture2DBindings == other.m_Texture2DBindings &&
         m_TextureCubeBindings == other.m_TextureCubeBindings &&
         m_RenderDataCategory == other.m_RenderDataCategory;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMaterialResource, 1, WRTTIDefaultAllocator<WMaterialResource>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_RESOURCE_IMPLEMENT_COMMON_CODE(WMaterialResource);
// clang-format on



WMaterialResource::WMaterialResource()
  : WResource(DoUpdate::OnGraphicsResourceThreads, 1)
{
}

WMaterialResource::~WMaterialResource()
{
  WMaterialManager::MaterialRemoved(this);
}

WHashedString WMaterialResource::GetPermutationValue(const WTempHashedString& sName)
{
  for (WUInt32 i = 0; i < m_mDesc.m_PermutationVars.GetCount(); ++i)
  {
    if (m_mDesc.m_PermutationVars[i].m_sName == sName)
    {
      return m_mDesc.m_PermutationVars[i].m_sValue;
      break;
    }
  }
  return {};
}

WHashedString WMaterialResource::GetSurface() const
{
  return m_mDesc.m_sSurface;
}

void WMaterialResource::SetParameter(const WHashedString& sName, const WVariant& value)
{
  if (SetProperty(m_mDesc.m_Parameters, sName, value, sName))
  {
    SetModified(DirtyFlags::Parameter);
  }
}

void WMaterialResource::SetParameter(const char* szName, const WVariant& value)
{
  WTempHashedString sName(szName);
  if (SetProperty(m_mDesc.m_Parameters, szName, value, sName))
  {
    SetModified(DirtyFlags::Parameter);
  }
}

WVariant WMaterialResource::GetParameter(const WTempHashedString& sName)
{
  return GetProperty<WVariant>(m_mDesc.m_Parameters, sName);
}

void WMaterialResource::SetTexture2DBinding(const WHashedString& sName, const WTexture2DResourceHandle& value)
{
  if (SetProperty(m_mDesc.m_Texture2DBindings, sName, value, sName))
  {
    SetModified(DirtyFlags::Texture2D);
  }
}

void WMaterialResource::SetTexture2DBinding(const char* szName, const WTexture2DResourceHandle& value)
{
  WTempHashedString sName(szName);
  if (SetProperty(m_mDesc.m_Texture2DBindings, szName, value, sName))
  {
    SetModified(DirtyFlags::Texture2D);
  }
}

WTexture2DResourceHandle WMaterialResource::GetTexture2DBinding(const WTempHashedString& sName)
{
  return GetProperty<WTexture2DResourceHandle>(m_mDesc.m_Texture2DBindings, sName);
}

void WMaterialResource::SetTextureCubeBinding(const WHashedString& sName, const WTextureCubeResourceHandle& value)
{
  if (SetProperty(m_mDesc.m_TextureCubeBindings, sName, value, sName))
  {
    SetModified(DirtyFlags::TextureCube);
  }
}

void WMaterialResource::SetTextureCubeBinding(const char* szName, const WTextureCubeResourceHandle& value)
{
  WTempHashedString sName(szName);
  if (SetProperty(m_mDesc.m_TextureCubeBindings, szName, value, sName))
  {
    SetModified(DirtyFlags::TextureCube);
  }
}

WTextureCubeResourceHandle WMaterialResource::GetTextureCubeBinding(const WTempHashedString& sName)
{
  return GetProperty<WTextureCubeResourceHandle>(m_mDesc.m_TextureCubeBindings, sName);
}

WRenderData::Category WMaterialResource::GetRenderDataCategory()
{
  return m_mDesc.m_RenderDataCategory;
}

// static
WRenderData::Category WMaterialResource::GetRenderDataCategory(const WMaterialResourceHandle& hMaterial, bool* out_pWasFallback /*= nullptr*/, WRenderData::Category fallbackCategory /*= WDefaultRenderDataCategories::LitOpaque*/)
{
  if (hMaterial.IsValid())
  {
    WResourceLock<WMaterialResource> pMaterial(hMaterial, WResourceAcquireMode::AllowLoadingFallback);
    if (out_pWasFallback != nullptr)
    {
      *out_pWasFallback = (pMaterial.GetAcquireResult() == WResourceAcquireResult::LoadingFallback);
    }

    return pMaterial->GetRenderDataCategory();
  }

  return fallbackCategory;
}

void WMaterialResource::PreserveCurrentDesc()
{
  m_mOriginalDesc = m_mDesc;
}

void WMaterialResource::ResetResource()
{
  if (m_mDesc != m_mOriginalDesc)
  {
    m_mDesc = m_mOriginalDesc;

    SetModified(DirtyFlags::ResourceReset);
  }
}

const char* WMaterialResource::GetDefaultMaterialFileName(DefaultMaterialType materialType)
{
  switch (materialType)
  {
    case DefaultMaterialType::Fullbright:
      return "Base/Materials/BaseMaterials/Fullbright.WMaterialAsset";
    case DefaultMaterialType::FullbrightAlphaTest:
      return "Base/Materials/BaseMaterials/FullbrightAlphaTest.WMaterialAsset";
    case DefaultMaterialType::Lit:
      return "Base/Materials/BaseMaterials/Lit.WMaterialAsset";
    case DefaultMaterialType::LitAlphaTest:
      return "Base/Materials/BaseMaterials/LitAlphaTest.WMaterialAsset";
    case DefaultMaterialType::Sky:
      return "Base/Materials/BaseMaterials/Sky.WMaterialAsset";
    case DefaultMaterialType::MissingMaterial:
      return "Base/Materials/Common/MissingMaterial.WMaterialAsset";
    default:
      W_ASSERT_NOT_IMPLEMENTED;
      return "";
  }
}

WResourceLoadDesc WMaterialResource::UnloadData(Unload WhatToUnload)
{
  m_mDesc.Clear();
  m_mOriginalDesc.Clear();

  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = WResourceState::Unloaded;

  return res;
}

WResourceLoadDesc WMaterialResource::UpdateContent(WStreamReader* pOuterStream)
{
  // Setting all dirty flags here outside of SetModified prevents the setters being used from calling into the WMaterialManager before the resource is fully loaded.
  m_DirtyFlags.SetValue(DirtyFlags::ResourceCreation);
  m_mDesc.Clear();
  m_mOriginalDesc.Clear();

  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = WResourceState::Loaded;

  if (pOuterStream == nullptr)
  {
    res.m_State = WResourceState::LoadedResourceMissing;
    return res;
  }

  WStringBuilder sAbsFilePath;
  (*pOuterStream) >> sAbsFilePath;

  WUInt8 uiVersion = 0;
  if (sAbsFilePath.HasExtension("WBinMaterial"))
  {
    WStringBuilder sTemp, sTemp2;

    WAssetFileHeader AssetHash;
    AssetHash.Read(*pOuterStream).IgnoreResult();

    (*pOuterStream) >> uiVersion;
    W_ASSERT_DEV(uiVersion >= 4 && uiVersion <= 8, "Unknown WBinMaterial version {0}", uiVersion);

    WUInt8 uiCompressionMode = 0;
    if (uiVersion >= 6)
    {
      *pOuterStream >> uiCompressionMode;
    }

    WStreamReader* pInnerStream = pOuterStream;

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
    WCompressedStreamReaderZstd decompressorZstd;
#endif

    switch (uiCompressionMode)
    {
      case 0:
        break;

      case 1:
#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
        decompressorZstd.SetInputStream(pOuterStream);
        pInnerStream = &decompressorZstd;
        break;
#else
        WLog::Error("Material resource is compressed with zstandard, but support for this compressor is not compiled in.");
        res.m_State = WResourceState::LoadedResourceMissing;
        return res;
#endif

      default:
        WLog::Error("Material resource is compressed with an unknown algorithm.");
        res.m_State = WResourceState::LoadedResourceMissing;
        return res;
    }

    WStreamReader& s = *pInnerStream;

    // Base material
    {
      s >> sTemp;

      if (!sTemp.IsEmpty())
        m_mDesc.m_hBaseMaterial = WResourceManager::LoadResource<WMaterialResource>(sTemp);
    }

    // Surface
    {
      s >> sTemp;
      m_mDesc.m_sSurface.Assign(sTemp.GetView());
    }

    // Shader
    {
      s >> sTemp;

      if (!sTemp.IsEmpty())
        m_mDesc.m_hShader = WResourceManager::LoadResource<WShaderResource>(sTemp);
    }

    // Permutation Variables
    {
      WUInt16 uiPermVars;
      s >> uiPermVars;

      m_mDesc.m_PermutationVars.Reserve(uiPermVars);

      for (WUInt16 i = 0; i < uiPermVars; ++i)
      {
        s >> sTemp;
        s >> sTemp2;

        if (!sTemp.IsEmpty() && !sTemp2.IsEmpty())
        {
          AddPermutationVar(sTemp, sTemp2);
        }
      }
    }

    // 2D Textures
    {
      WUInt16 uiTextures = 0;
      s >> uiTextures;

      m_mDesc.m_Texture2DBindings.Reserve(uiTextures);

      for (WUInt16 i = 0; i < uiTextures; ++i)
      {
        s >> sTemp;
        s >> sTemp2;

        if (!sTemp.IsEmpty() && !sTemp2.IsEmpty())
        {
          WMaterialResourceDescriptor::Texture2DBinding& tc = m_mDesc.m_Texture2DBindings.ExpandAndGetRef();
          tc.m_Name.Assign(sTemp.GetData());
          tc.m_Value = WResourceManager::LoadResource<WTexture2DResource>(sTemp2);
        }
      }
    }

    // Cube Textures
    {
      WUInt16 uiTextures = 0;
      s >> uiTextures;

      m_mDesc.m_TextureCubeBindings.Reserve(uiTextures);

      for (WUInt16 i = 0; i < uiTextures; ++i)
      {
        s >> sTemp;
        s >> sTemp2;

        if (!sTemp.IsEmpty() && !sTemp2.IsEmpty())
        {
          WMaterialResourceDescriptor::TextureCubeBinding& tc = m_mDesc.m_TextureCubeBindings.ExpandAndGetRef();
          tc.m_Name.Assign(sTemp.GetData());
          tc.m_Value = WResourceManager::LoadResource<WTextureCubeResource>(sTemp2);
        }
      }
    }

    // Shader constants
    {
      WUInt16 uiConstants = 0;
      s >> uiConstants;

      m_mDesc.m_Parameters.Reserve(uiConstants);

      WVariant vTemp;

      for (WUInt16 i = 0; i < uiConstants; ++i)
      {
        s >> sTemp;
        s >> vTemp;

        if (!sTemp.IsEmpty() && vTemp.IsValid())
        {
          WMaterialResourceDescriptor::Parameter& tc = m_mDesc.m_Parameters.ExpandAndGetRef();
          tc.m_Name.Assign(sTemp.GetData());
          tc.m_Value = vTemp;
        }
      }
    }

    // Render data category
    if (uiVersion >= 7)
    {
      WStringBuilder sRenderDataCategoryName;
      s >> sRenderDataCategoryName;

      WTempHashedString sCategoryNameHashed(sRenderDataCategoryName.GetView());
      if (sCategoryNameHashed != WTempHashedString("<Invalid>"))
      {
        m_mDesc.m_RenderDataCategory = WRenderData::FindCategory(sCategoryNameHashed);
        if (m_mDesc.m_RenderDataCategory == WInvalidRenderDataCategory)
        {
          WLog::Error("Material '{}' uses an invalid render data category '{}'", GetResourceIdOrDescription(), sRenderDataCategoryName);
        }
      }
    }

    if (uiVersion >= 5)
    {
      WStreamReader& s = *pInnerStream;

      WStringBuilder sResourceName;
      s >> sResourceName;

      WTextureResourceLoader::LoadedData embedded;

      while (!sResourceName.IsEmpty())
      {
        WUInt32 dataSize = 0;
        s >> dataSize;

        WTextureResourceLoader::LoadTexFile(s, embedded).IgnoreResult();
        embedded.m_bIsFallback = true;

        WDefaultMemoryStreamStorage storage;
        WMemoryStreamWriter loadStreamWriter(&storage);
        WTextureResourceLoader::WriteTextureLoadStream(loadStreamWriter, embedded);

        WMemoryStreamReader loadStreamReader(&storage);

        WTexture2DResourceHandle hTexture = WResourceManager::LoadResource<WTexture2DResource>(sResourceName);
        WResourceManager::SetResourceLowResData(hTexture, &loadStreamReader);

        s >> sResourceName;
      }
    }
  }
  else if (sAbsFilePath.HasExtension("WMaterial"))
  {
    WOpenDdlReader reader;

    if (reader.ParseDocument(*pOuterStream, 0, WLog::GetThreadLocalLogSystem()).Failed())
    {
      res.m_State = WResourceState::LoadedResourceMissing;
      return res;
    }

    const WOpenDdlReaderElement* pRoot = reader.GetRootElement();

    // Read the base material
    if (const WOpenDdlReaderElement* pBase = pRoot->FindChildOfType(WOpenDdlPrimitiveType::String, "BaseMaterial"))
    {
      m_mDesc.m_hBaseMaterial = WResourceManager::LoadResource<WMaterialResource>(pBase->GetPrimitivesString()[0]);
    }

    // Read the shader
    if (const WOpenDdlReaderElement* pShader = pRoot->FindChildOfType(WOpenDdlPrimitiveType::String, "Shader"))
    {
      m_mDesc.m_hShader = WResourceManager::LoadResource<WShaderResource>(pShader->GetPrimitivesString()[0]);
    }

    // Read the render data category
    if (const WOpenDdlReaderElement* pRenderDataCategory = pRoot->FindChildOfType(WOpenDdlPrimitiveType::String, "RenderDataCategory"))
    {
      m_mDesc.m_RenderDataCategory = WRenderData::FindCategory(WTempHashedString(pRenderDataCategory->GetPrimitivesString()[0]));
    }

    for (const WOpenDdlReaderElement* pChild = pRoot->GetFirstChild(); pChild != nullptr; pChild = pChild->GetSibling())
    {
      // Read the shader permutation variables
      if (pChild->IsCustomType("Permutation"))
      {
        const WOpenDdlReaderElement* pName = pChild->FindChildOfType(WOpenDdlPrimitiveType::String, "Variable");
        const WOpenDdlReaderElement* pValue = pChild->FindChildOfType(WOpenDdlPrimitiveType::String, "Value");

        if (pName && pValue)
        {
          AddPermutationVar(pName->GetPrimitivesString()[0], pValue->GetPrimitivesString()[0]);
        }
      }

      // Read the shader constants
      if (pChild->IsCustomType("Constant"))
      {
        const WOpenDdlReaderElement* pName = pChild->FindChildOfType(WOpenDdlPrimitiveType::String, "Variable");
        const WOpenDdlReaderElement* pValue = pChild->FindChild("Value");

        WVariant value;
        if (pName && pValue && WOpenDdlUtils::ConvertToVariant(pValue, value).Succeeded())
        {
          WMaterialResourceDescriptor::Parameter& sc = m_mDesc.m_Parameters.ExpandAndGetRef();
          sc.m_Name.Assign(pName->GetPrimitivesString()[0]);
          sc.m_Value = value;
        }
      }

      // Read the texture references
      if (pChild->IsCustomType("Texture2D"))
      {
        const WOpenDdlReaderElement* pName = pChild->FindChildOfType(WOpenDdlPrimitiveType::String, "Variable");
        const WOpenDdlReaderElement* pValue = pChild->FindChildOfType(WOpenDdlPrimitiveType::String, "Value");

        if (pName && pValue)
        {
          WMaterialResourceDescriptor::Texture2DBinding& tc = m_mDesc.m_Texture2DBindings.ExpandAndGetRef();
          tc.m_Name.Assign(pName->GetPrimitivesString()[0]);
          tc.m_Value = WResourceManager::LoadResource<WTexture2DResource>(pValue->GetPrimitivesString()[0]);
        }
      }

      // Read the texture references
      if (pChild->IsCustomType("TextureCube"))
      {
        const WOpenDdlReaderElement* pName = pChild->FindChildOfType(WOpenDdlPrimitiveType::String, "Variable");
        const WOpenDdlReaderElement* pValue = pChild->FindChildOfType(WOpenDdlPrimitiveType::String, "Value");

        if (pName && pValue)
        {
          WMaterialResourceDescriptor::TextureCubeBinding& tc = m_mDesc.m_TextureCubeBindings.ExpandAndGetRef();
          tc.m_Name.Assign(pName->GetPrimitivesString()[0]);
          tc.m_Value = WResourceManager::LoadResource<WTextureCubeResource>(pValue->GetPrimitivesString()[0]);
        }
      }
    }
  }
  else
  {
    WLog::Error("Unknown material file type: '{}'", sAbsFilePath);
  }

  // With version 8, all materials are flattened at asset transform time, removing the need to flatten the base material hierarchy.
  if (uiVersion < 8)
  {
    // Flatten works on the original desc of the base material hierarchy and stores the end result in m_mDesc.
    m_mOriginalDesc = m_mDesc;
    FlattenOriginalDescHierarchy();
  }

  // There is no guarantee that a material defines a render data category, so we always have to compute the fallbacks.
  ComputeRenderDataCategory();

  // After loading, base material info is removed as everything is flattened into this material.
  m_mDesc.m_hBaseMaterial.Invalidate();
  W_ASSERT_DEBUG(m_mDesc.m_RenderDataCategory != WInvalidRenderDataCategory, "FlattenHierarchy should have set a category and newer versions should have it serialized.");

  m_mOriginalDesc = m_mDesc;

  // We add the material right away instead of during extraction / begin rendering to make sure the materialId can be used right away.
  WMaterialManager::MaterialAdded(this);
  return res;
}

void WMaterialResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU =
    sizeof(WMaterialResource) + (WUInt32)(m_mDesc.m_PermutationVars.GetHeapMemoryUsage() + m_mDesc.m_Parameters.GetHeapMemoryUsage() + m_mDesc.m_Texture2DBindings.GetHeapMemoryUsage() + m_mDesc.m_TextureCubeBindings.GetHeapMemoryUsage() + m_mOriginalDesc.m_PermutationVars.GetHeapMemoryUsage() + m_mOriginalDesc.m_Parameters.GetHeapMemoryUsage() + m_mOriginalDesc.m_Texture2DBindings.GetHeapMemoryUsage() + m_mOriginalDesc.m_TextureCubeBindings.GetHeapMemoryUsage());

  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

W_RESOURCE_IMPLEMENT_CREATEABLE(WMaterialResource, WMaterialResourceDescriptor)
{
  m_mOriginalDesc = descriptor;

  WResourceLoadDesc res;
  res.m_State = WResourceState::Loaded;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  m_DirtyFlags = DirtyFlags::ResourceCreation;
  FlattenOriginalDescHierarchy();
  ComputeRenderDataCategory();

  // After creation, base material info is removed as everything is flattened into this material.
  m_mDesc.m_hBaseMaterial.Invalidate();
  W_ASSERT_DEBUG(m_mDesc.m_RenderDataCategory != WInvalidRenderDataCategory, "FlattenHierarchy should have set a category");
  m_mOriginalDesc = m_mDesc;

  // We add the material right away instead of during extraction / begin rendering to make sure the materialId can be used right away.
  WMaterialManager::MaterialAdded(this);
  return res;
}

void WMaterialResource::AddPermutationVar(WStringView sName, WStringView sValue)
{
  WHashedString sNameHashed;
  sNameHashed.Assign(sName);
  WHashedString sValueHashed;
  sValueHashed.Assign(sValue);

  if (WShaderManager::IsPermutationValueAllowed(sNameHashed, sValueHashed))
  {
    WPermutationVar& pv = m_mDesc.m_PermutationVars.ExpandAndGetRef();
    pv.m_sName = sNameHashed;
    pv.m_sValue = sValueHashed;
  }
  SetModified(DirtyFlags::PermutationVar);
}

void WMaterialResource::SetModified(WMaterialResource::DirtyFlags::Enum flag)
{
  bool bAlreadyModified = m_DirtyFlags.IsAnyFlagSet();
  m_DirtyFlags |= flag;
  if (!bAlreadyModified)
  {
    WMaterialManager::MaterialModified(GetResourceHandle());
  }
  m_ModifiedEvent.Broadcast(this);
}


void WMaterialResource::FlattenOriginalDescHierarchy()
{
  WTempHybridArray<WMaterialResource*, 16> materialHierarchy;
  WMaterialResource* pCurrentMaterial = this;

  while (true)
  {
    materialHierarchy.PushBack(pCurrentMaterial);

    const WMaterialResourceHandle& hBaseMaterial = pCurrentMaterial->m_mOriginalDesc.m_hBaseMaterial;
    if (!hBaseMaterial.IsValid())
      break;

    // Ensure that the base material is loaded at this point.
    // For loaded materials this will always be the case but is still necessary for runtime created materials.
    pCurrentMaterial = WResourceManager::BeginAcquireResource(hBaseMaterial, WResourceAcquireMode::BlockTillLoaded);
  }

  W_SCOPE_EXIT(for (WUInt32 i = materialHierarchy.GetCount(); i-- > 1;) {
    WMaterialResource* pMaterial = materialHierarchy[i];
    WResourceManager::EndAcquireResource(pMaterial);

    materialHierarchy[i] = nullptr;
  });

  struct FlattenedMaterial
  {
    WShaderResourceHandle m_hShader;
    WHashedString m_sSurface;
    WHashTable<WHashedString, WHashedString> m_PermutationVars;
    WHashTable<WHashedString, WVariant> m_Parameters;
    WHashTable<WHashedString, WTexture2DResourceHandle> m_Texture2DBindings;
    WHashTable<WHashedString, WTextureCubeResourceHandle> m_TextureCubeBindings;
    WRenderData::Category m_RenderDataCategory;
  } flattenedMaterial;

  // set state of parent material first
  for (WUInt32 i = materialHierarchy.GetCount(); i-- > 0;)
  {
    WMaterialResource* pMaterial = materialHierarchy[i];
    const WMaterialResourceDescriptor& desc = pMaterial->m_mOriginalDesc;

    if (desc.m_hShader.IsValid())
      flattenedMaterial.m_hShader = desc.m_hShader;

    if (!desc.m_sSurface.IsEmpty())
      flattenedMaterial.m_sSurface = desc.m_sSurface;

    for (const auto& permutationVar : desc.m_PermutationVars)
    {
      flattenedMaterial.m_PermutationVars.Insert(permutationVar.m_sName, permutationVar.m_sValue);
    }

    for (const auto& param : desc.m_Parameters)
    {
      flattenedMaterial.m_Parameters.Insert(param.m_Name, param.m_Value);
    }

    for (const auto& textureBinding : desc.m_Texture2DBindings)
    {
      flattenedMaterial.m_Texture2DBindings.Insert(textureBinding.m_Name, textureBinding.m_Value);
    }

    for (const auto& textureBinding : desc.m_TextureCubeBindings)
    {
      flattenedMaterial.m_TextureCubeBindings.Insert(textureBinding.m_Name, textureBinding.m_Value);
    }

    if (desc.m_RenderDataCategory != WInvalidRenderDataCategory)
    {
      flattenedMaterial.m_RenderDataCategory = desc.m_RenderDataCategory;
    }
  }

  m_mDesc.m_hBaseMaterial.Invalidate();
  m_mDesc.m_sSurface = flattenedMaterial.m_sSurface;
  m_mDesc.m_hShader = flattenedMaterial.m_hShader;
  m_mDesc.m_RenderDataCategory = flattenedMaterial.m_RenderDataCategory;
  CopyMaterialDesc(flattenedMaterial.m_PermutationVars, m_mDesc.m_PermutationVars);
  CopyMaterialDesc(flattenedMaterial.m_Parameters, m_mDesc.m_Parameters);
  CopyMaterialDesc(flattenedMaterial.m_Texture2DBindings, m_mDesc.m_Texture2DBindings);
  CopyMaterialDesc(flattenedMaterial.m_TextureCubeBindings, m_mDesc.m_TextureCubeBindings);
}

void WMaterialResource::ComputeRenderDataCategory()
{
  if (m_mDesc.m_RenderDataCategory.IsValid())
    return;

  WHashedString sBlendModeValue = GetPermutationValue("BLEND_MODE");
  if (sBlendModeValue.IsEmpty() || sBlendModeValue == WTempHashedString("BLEND_MODE_OPAQUE"))
  {
    m_mDesc.m_RenderDataCategory = WDefaultRenderDataCategories::LitOpaque;
  }
  else if (sBlendModeValue == WTempHashedString("BLEND_MODE_MASKED") || sBlendModeValue == WTempHashedString("BLEND_MODE_DITHERED"))
  {
    m_mDesc.m_RenderDataCategory = WDefaultRenderDataCategories::LitMasked;
  }
  else
  {
    m_mDesc.m_RenderDataCategory = WDefaultRenderDataCategories::LitTransparent;
  }
}

const WMaterialResourceDescriptor& WMaterialResource::GetCurrentDesc() const
{
  return m_mDesc;
}

W_STATICLINK_FILE(RendererCore, RendererCore_Material_Implementation_MaterialResource);
