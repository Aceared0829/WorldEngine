#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/TextureAsset/TextureAsset.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetManager.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetWindow.moc.h>
#include <ToolsFoundation/Assets/AssetFileExtensionWhitelist.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WTextureAssetProfileConfig, 1, WRTTIDefaultAllocator<WTextureAssetProfileConfig>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("MaxResolution", m_uiMaxResolution)->AddAttributes(new WDefaultValueAttribute(16 * 1024)),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WTextureAssetDocumentManager, 1, WRTTIDefaultAllocator<WTextureAssetDocumentManager>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WTextureAssetDocumentManager::WTextureAssetDocumentManager()
{
  WDocumentManager::s_Events.AddEventHandler(WMakeDelegate(&WTextureAssetDocumentManager::OnDocumentManagerEvent, this));

  // additional whitelist for non-asset files where an asset may be selected
  WAssetFileExtensionWhitelist::AddAssetFileExtension("CompatibleAsset_Texture_2D", "dds");
  WAssetFileExtensionWhitelist::AddAssetFileExtension("CompatibleAsset_Texture_2D", "color");

  // texture asset source files
  WAssetFileExtensionWhitelist::AddAssetFileExtension("Image2D", "dds");
  WAssetFileExtensionWhitelist::AddAssetFileExtension("Image2D", "tga");

  m_DocTypeDesc.m_sDocumentTypeName = "Texture 2D";
  m_DocTypeDesc.m_sFileExtension = "WTextureAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/Texture_2D.svg";
  m_DocTypeDesc.m_sAssetCategory = "Rendering";
  m_DocTypeDesc.m_pDocumentType = WGetStaticRTTI<WTextureAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_sResourceFileExtension = "WBinTexture2D";
  m_DocTypeDesc.m_AssetDocumentFlags = WAssetDocumentFlags::AutoThumbnailOnTransform;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Texture_2D");

  m_DocTypeDesc2.m_sDocumentTypeName = "Render Target";
  m_DocTypeDesc2.m_sFileExtension = "WRenderTargetAsset";
  m_DocTypeDesc2.m_sIcon = ":/AssetIcons/Render_Target.svg";
  m_DocTypeDesc2.m_sAssetCategory = "Rendering";
  m_DocTypeDesc2.m_pDocumentType = WGetStaticRTTI<WTextureAssetDocument>();
  m_DocTypeDesc2.m_pManager = this;
  m_DocTypeDesc2.m_sResourceFileExtension = "WBinRenderTarget";
  m_DocTypeDesc2.m_AssetDocumentFlags = WAssetDocumentFlags::AutoTransformOnSave;
  m_DocTypeDesc2.m_CompatibleTypes.PushBack("CompatibleAsset_Texture_2D"); // render targets can also be used as 2D textures
  m_DocTypeDesc2.m_CompatibleTypes.PushBack("CompatibleAsset_Texture_Target");

  WQtImageCache::GetSingleton()->RegisterTypeImage("Render Target", QPixmap(":/AssetIcons/Render_Target.svg"));
}

WTextureAssetDocumentManager::~WTextureAssetDocumentManager()
{
  WDocumentManager::s_Events.RemoveEventHandler(WMakeDelegate(&WTextureAssetDocumentManager::OnDocumentManagerEvent, this));
}

WUInt64 WTextureAssetDocumentManager::ComputeAssetProfileHashImpl(const WPlatformProfile* pAssetProfile) const
{
  return pAssetProfile->GetTypeConfig<WTextureAssetProfileConfig>()->m_uiMaxResolution;
}

void WTextureAssetDocumentManager::OnDocumentManagerEvent(const WDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case WDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == WGetStaticRTTI<WTextureAssetDocument>())
      {
        new WQtTextureAssetDocumentWindow(static_cast<WTextureAssetDocument*>(e.m_pDocument)); // NOLINT: Not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void WTextureAssetDocumentManager::InternalCreateDocument(WStringView sDocumentTypeName, WStringView sPath, bool bCreateNewDocument, WDocument*& out_pDocument, const WDocumentObject* pOpenContext)
{
  WTextureAssetDocument* pDoc = new WTextureAssetDocument(sPath);
  out_pDocument = pDoc;

  if (sDocumentTypeName.IsEqual("Render Target"))
  {
    pDoc->m_bIsRenderTarget = true;
  }
}

void WTextureAssetDocumentManager::InternalGetSupportedDocumentTypes(WDynamicArray<const WDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
  inout_DocumentTypes.PushBack(&m_DocTypeDesc2);
}

WString WTextureAssetDocumentManager::GetRelativeOutputFileName(const WAssetDocumentTypeDescriptor* pTypeDescriptor, WStringView sDataDirectory, WStringView sDocumentPath, WStringView sOutputTag, const WPlatformProfile* pAssetProfile) const
{
  if (sOutputTag.IsEqual("LOWRES"))
  {
    WStringBuilder sRelativePath(sDocumentPath);
    sRelativePath.MakeRelativeTo(sDataDirectory).IgnoreResult();
    sRelativePath.RemoveFileExtension();
    sRelativePath.Append("-lowres");
    sRelativePath.Append(".ext"); // dummy extension, so that the next function knows which part to modify
    WAssetDocumentManager::GenerateOutputFilename(sRelativePath, pAssetProfile, "WBinTexture2D", true);
    return sRelativePath;
  }

  return SUPER::GetRelativeOutputFileName(pTypeDescriptor, sDataDirectory, sDocumentPath, sOutputTag, pAssetProfile);
}

void WTextureAssetDocumentManager::AppendAssetInfoSummary(WStringBuilder& ref_sOut, const WAssetInfoFile& info, WStringView sLinePrefix) const
{
  const WStringView keys[] = {WAssetInfoFile::Keys::ImageWidth, WAssetInfoFile::Keys::Format};
  info.AppendValuesToDisplayString(ref_sOut, keys, sLinePrefix);
}
