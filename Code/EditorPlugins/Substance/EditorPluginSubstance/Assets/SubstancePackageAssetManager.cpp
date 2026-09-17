#include <EditorPluginSubstance/EditorPluginSubstancePCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginSubstance/Assets/SubstancePackageAsset.h>
#include <EditorPluginSubstance/Assets/SubstancePackageAssetManager.h>
#include <EditorPluginSubstance/Assets/SubstancePackageAssetWindow.moc.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSubstancePackageAssetDocumentManager, 1, WRTTIDefaultAllocator<WSubstancePackageAssetDocumentManager>)
W_END_DYNAMIC_REFLECTED_TYPE;

WSubstancePackageAssetDocumentManager::WSubstancePackageAssetDocumentManager()
{
  WDocumentManager::s_Events.AddEventHandler(WMakeDelegate(&WSubstancePackageAssetDocumentManager::OnDocumentManagerEvent, this));

  {
    m_PackageTypeDesc.m_sDocumentTypeName = "Substance Package";
    m_PackageTypeDesc.m_sFileExtension = "WSubstancePackageAsset";
    m_PackageTypeDesc.m_sIcon = ":/AssetIcons/SubstanceDesigner.svg";
    m_PackageTypeDesc.m_sAssetCategory = "Rendering";
    m_PackageTypeDesc.m_pDocumentType = WGetStaticRTTI<WSubstancePackageAssetDocument>();
    m_PackageTypeDesc.m_pManager = this;
    m_PackageTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Substance_Package");

    m_PackageTypeDesc.m_sResourceFileExtension = "WBinSubstancePackage";
    m_PackageTypeDesc.m_AssetDocumentFlags = WAssetDocumentFlags::SubAssetsAutoThumbnailOnTransform;

    WQtImageCache::GetSingleton()->RegisterTypeImage("Substance Package", QPixmap(":/AssetIcons/SubstanceDesigner.svg"));
  }

  {
    m_TextureTypeDesc.m_bCanCreate = false;
    m_TextureTypeDesc.m_sDocumentTypeName = "Substance Texture";
    m_TextureTypeDesc.m_sFileExtension = "WSubstanceTextureAsset";
    m_TextureTypeDesc.m_sIcon = ":/AssetIcons/SubstanceDesigner.svg";
    m_TextureTypeDesc.m_sAssetCategory = "Rendering";
    m_TextureTypeDesc.m_pManager = this;
    m_TextureTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Texture_2D");

    m_TextureTypeDesc.m_sResourceFileExtension = "WBinTexture2D";
    m_TextureTypeDesc.m_AssetDocumentFlags = WAssetDocumentFlags::AutoThumbnailOnTransform;
  }
}

WSubstancePackageAssetDocumentManager::~WSubstancePackageAssetDocumentManager()
{
  WDocumentManager::s_Events.RemoveEventHandler(WMakeDelegate(&WSubstancePackageAssetDocumentManager::OnDocumentManagerEvent, this));
}

void WSubstancePackageAssetDocumentManager::FillOutSubAssetList(const WAssetDocumentInfo& assetInfo, WDynamicArray<WSubAssetData>& out_subAssets) const
{
  auto pMetaData = assetInfo.GetMetaInfo<WSubstancePackageAssetMetaData>();
  if (pMetaData == nullptr)
    return;

  for (WUInt32 i = 0; i < pMetaData->m_OutputUuids.GetCount(); ++i)
  {
    auto& subAsset = out_subAssets.ExpandAndGetRef();
    subAsset.m_Guid = pMetaData->m_OutputUuids[i];
    subAsset.m_sName = pMetaData->m_OutputNames[i];
    subAsset.m_sSubAssetsDocumentTypeName.Assign(m_TextureTypeDesc.m_sDocumentTypeName);
  }
}

WString WSubstancePackageAssetDocumentManager::GetAssetTableEntry(const WSubAsset* pSubAsset, WStringView sDataDirectory, const WPlatformProfile* pAssetProfile) const
{
  if (pSubAsset->m_bMainAsset)
  {
    return SUPER::GetAssetTableEntry(pSubAsset, sDataDirectory, pAssetProfile);
  }

  WStringBuilder sTargetFile = pSubAsset->m_pAssetInfo->m_Path.GetAbsolutePath().GetFileDirectory();
  sTargetFile.Append(pSubAsset->m_Data.m_sName);

  return GetRelativeOutputFileName(&m_TextureTypeDesc, sDataDirectory, sTargetFile, "", pAssetProfile);
}

void WSubstancePackageAssetDocumentManager::OnDocumentManagerEvent(const WDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case WDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == WGetStaticRTTI<WSubstancePackageAssetDocument>())
      {
        new WQtSubstancePackageAssetWindow(static_cast<WSubstancePackageAssetDocument*>(e.m_pDocument)); // NOLINT: Not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void WSubstancePackageAssetDocumentManager::InternalCreateDocument(WStringView sDocumentTypeName, WStringView sPath, bool bCreateNewDocument, WDocument*& out_pDocument, const WDocumentObject* pOpenContext)
{
  out_pDocument = new WSubstancePackageAssetDocument(sPath);
}

void WSubstancePackageAssetDocumentManager::InternalGetSupportedDocumentTypes(WDynamicArray<const WDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_PackageTypeDesc);
  inout_DocumentTypes.PushBack(&m_TextureTypeDesc);
}
