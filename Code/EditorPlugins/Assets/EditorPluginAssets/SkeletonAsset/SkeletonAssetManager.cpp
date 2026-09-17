#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/SkeletonAsset/SkeletonAssetManager.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonAssetWindow.moc.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSkeletonAssetDocumentManager, 1, WRTTIDefaultAllocator<WSkeletonAssetDocumentManager>)
W_END_DYNAMIC_REFLECTED_TYPE;

WSkeletonAssetDocumentManager::WSkeletonAssetDocumentManager()
{
  WDocumentManager::s_Events.AddEventHandler(WMakeDelegate(&WSkeletonAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "Skeleton";
  m_DocTypeDesc.m_sFileExtension = "WSkeletonAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/Skeleton.svg";
  m_DocTypeDesc.m_sAssetCategory = "Animation";
  m_DocTypeDesc.m_pDocumentType = WGetStaticRTTI<WSkeletonAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Mesh_Skeleton");

  m_DocTypeDesc.m_sResourceFileExtension = "WBinSkeleton";
  m_DocTypeDesc.m_AssetDocumentFlags = WAssetDocumentFlags::SupportsThumbnail | WAssetDocumentFlags::AutoTransformOnSave;
}

WSkeletonAssetDocumentManager::~WSkeletonAssetDocumentManager()
{
  WDocumentManager::s_Events.RemoveEventHandler(WMakeDelegate(&WSkeletonAssetDocumentManager::OnDocumentManagerEvent, this));
}

void WSkeletonAssetDocumentManager::OnDocumentManagerEvent(const WDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case WDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == WGetStaticRTTI<WSkeletonAssetDocument>())
      {
        new WQtSkeletonAssetDocumentWindow(static_cast<WSkeletonAssetDocument*>(e.m_pDocument)); // NOLINT: Not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void WSkeletonAssetDocumentManager::InternalCreateDocument(
  WStringView sDocumentTypeName, WStringView sPath, bool bCreateNewDocument, WDocument*& out_pDocument, const WDocumentObject* pOpenContext)
{
  out_pDocument = new WSkeletonAssetDocument(sPath);
}

void WSkeletonAssetDocumentManager::InternalGetSupportedDocumentTypes(WDynamicArray<const WDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}

void WSkeletonAssetDocumentManager::AppendAssetInfoSummary(WStringBuilder& ref_sOut, const WAssetInfoFile& info, WStringView sLinePrefix) const
{
  const WStringView keys[] = {WAssetInfoFile::Keys::NumBones};
  info.AppendValuesToDisplayString(ref_sOut, keys, sLinePrefix);
}
