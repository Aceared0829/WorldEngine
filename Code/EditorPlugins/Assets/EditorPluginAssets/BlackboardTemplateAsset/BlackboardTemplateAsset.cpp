#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocumentInfo.h>
#include <EditorPluginAssets/BlackboardTemplateAsset/BlackboardTemplateAsset.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WBlackboardTemplateAssetObject, 1, WRTTIDefaultAllocator<WBlackboardTemplateAssetObject>)
{
  W_BEGIN_PROPERTIES
  {
    W_ARRAY_MEMBER_PROPERTY("BaseTemplates", m_BaseTemplates)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_BlackboardTemplate", WDependencyFlags::Transform)),
    W_ARRAY_MEMBER_PROPERTY("Entries", m_Entries),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WBlackboardTemplateAssetDocument, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WBlackboardTemplateAssetDocument::WBlackboardTemplateAssetDocument(WStringView sDocumentPath)
  : WSimpleAssetDocument<WBlackboardTemplateAssetObject>(sDocumentPath, WAssetDocEngineConnection::None)
{
}

WStatus WBlackboardTemplateAssetDocument::WriteAsset(WStreamWriter& inout_stream, const WPlatformProfile* pAssetProfile) const
{
  WBlackboardTemplateResourceDescriptor desc;
  W_SUCCEED_OR_RETURN(RetrieveState(GetProperties(), desc));
  W_SUCCEED_OR_RETURN(desc.Serialize(inout_stream));

  return WStatus(W_SUCCESS);
}

WStatus WBlackboardTemplateAssetDocument::RetrieveState(const WBlackboardTemplateAssetObject* pProp, WBlackboardTemplateResourceDescriptor& inout_Desc) const
{
  for (const WString& sTempl : pProp->m_BaseTemplates)
  {
    if (sTempl.IsEmpty())
      continue;

    auto pOther = WAssetCurator::GetSingleton()->FindSubAsset(sTempl);
    if (!pOther.isValid())
    {
      return WStatus(WFmt("Base template '{}' not found.", sTempl));
    }

    WDocument* pDoc;
    W_SUCCEED_OR_RETURN(pOther->m_pAssetInfo->GetManager()->OpenDocument(pOther->m_Data.m_sSubAssetsDocumentTypeName, pOther->m_pAssetInfo->m_Path, pDoc, WDocumentFlags::None, nullptr));

    if (WBlackboardTemplateAssetDocument* pTmpDoc = WDynamicCast<WBlackboardTemplateAssetDocument*>(pDoc))
    {
      W_SUCCEED_OR_RETURN(RetrieveState(pTmpDoc->GetProperties(), inout_Desc));
    }

    pOther->m_pAssetInfo->GetManager()->CloseDocument(pDoc);
  }

  for (const auto& e : pProp->m_Entries)
  {
    for (auto& e2 : inout_Desc.m_Entries)
    {
      if (e2.m_sName == e.m_sName)
      {
        e2 = e;
        goto next;
      }
    }

    inout_Desc.m_Entries.PushBack(e);

  next:;
  }

  return WStatus(W_SUCCESS);
}

WTransformStatus WBlackboardTemplateAssetDocument::InternalTransformAsset(WStreamWriter& inout_stream, WStringView sOutputTag, const WPlatformProfile* pAssetProfile, const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags)
{
  return WriteAsset(inout_stream, pAssetProfile);
}
