#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocumentInfo.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WAssetDocumentInfo, 2, WRTTIDefaultAllocator<WAssetDocumentInfo>)
{
  W_BEGIN_PROPERTIES
  {
    W_SET_MEMBER_PROPERTY("Dependencies", m_TransformDependencies),
    W_SET_MEMBER_PROPERTY("References", m_ThumbnailDependencies),
    W_SET_MEMBER_PROPERTY("PackageDeps", m_PackageDependencies),
    W_SET_MEMBER_PROPERTY("Outputs", m_Outputs),
    W_MEMBER_PROPERTY("Hash", m_uiSettingsHash),
    W_ACCESSOR_PROPERTY("AssetType", GetAssetsDocumentTypeName, SetAssetsDocumentTypeName),
    W_ACCESSOR_PROPERTY("Tags", GetAssetsDocumentTags, SetAssetsDocumentTags),
    W_ARRAY_MEMBER_PROPERTY("MetaInfo", m_MetaInfo)->AddFlags(WPropertyFlags::PointerOwner),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WAssetDocumentInfo::WAssetDocumentInfo()
{
  m_uiSettingsHash = 0;
}

WAssetDocumentInfo::~WAssetDocumentInfo()
{
  ClearMetaData();
}

WAssetDocumentInfo::WAssetDocumentInfo(WAssetDocumentInfo&& rhs)
{
  (*this) = std::move(rhs);
}

void WAssetDocumentInfo::operator=(WAssetDocumentInfo&& rhs)
{
  m_uiSettingsHash = rhs.m_uiSettingsHash;
  m_TransformDependencies = rhs.m_TransformDependencies;
  m_ThumbnailDependencies = rhs.m_ThumbnailDependencies;
  m_PackageDependencies = rhs.m_PackageDependencies;
  m_Outputs = rhs.m_Outputs;
  m_sAssetsDocumentTypeName = rhs.m_sAssetsDocumentTypeName;
  m_sAssetsDocumentTags = rhs.m_sAssetsDocumentTags;
  m_MetaInfo = std::move(rhs.m_MetaInfo);
}

void WAssetDocumentInfo::CreateShallowClone(WAssetDocumentInfo& rhs) const
{
  rhs.m_uiSettingsHash = m_uiSettingsHash;
  rhs.m_TransformDependencies = m_TransformDependencies;
  rhs.m_ThumbnailDependencies = m_ThumbnailDependencies;
  rhs.m_PackageDependencies = m_PackageDependencies;
  rhs.m_Outputs = m_Outputs;
  rhs.m_sAssetsDocumentTypeName = m_sAssetsDocumentTypeName;
  rhs.m_sAssetsDocumentTags = m_sAssetsDocumentTags;
  rhs.m_MetaInfo.Clear();
}

void WAssetDocumentInfo::ClearMetaData()
{
  for (auto* pObj : m_MetaInfo)
  {
    if (pObj)
    {
      pObj->GetDynamicRTTI()->GetAllocator()->Deallocate(pObj);
    }
  }
  m_MetaInfo.Clear();
}

const char* WAssetDocumentInfo::GetAssetsDocumentTypeName() const
{
  return m_sAssetsDocumentTypeName.GetData();
}

const WString& WAssetDocumentInfo::GetAssetsDocumentTags() const
{
  return m_sAssetsDocumentTags;
}

void WAssetDocumentInfo::SetAssetsDocumentTypeName(const char* szSz)
{
  m_sAssetsDocumentTypeName.Assign(szSz);
}

void WAssetDocumentInfo::SetAssetsDocumentTags(const WString& sTags)
{
  m_sAssetsDocumentTags = sTags;
}

const WReflectedClass* WAssetDocumentInfo::GetMetaInfo(const WRTTI* pType) const
{
  for (auto* pObj : m_MetaInfo)
  {
    if (pObj->GetDynamicRTTI()->IsDerivedFrom(pType))
      return pObj;
  }
  return nullptr;
}
