#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include "ToolsFoundation/Assets/AssetFileExtensionWhitelist.h"
#include <EditorPluginAssets/TextureCubeAsset/TextureCubeAsset.h>
#include <EditorPluginAssets/TextureCubeAsset/TextureCubeAssetManager.h>
#include <EditorPluginAssets/TextureCubeAsset/TextureCubeAssetWindow.moc.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WTextureCubeAssetDocumentManager, 1, WRTTIDefaultAllocator<WTextureCubeAssetDocumentManager>)
W_END_DYNAMIC_REFLECTED_TYPE;

WTextureCubeAssetDocumentManager::WTextureCubeAssetDocumentManager()
{
  WDocumentManager::s_Events.AddEventHandler(WMakeDelegate(&WTextureCubeAssetDocumentManager::OnDocumentManagerEvent, this));

  // additional whitelist for non-asset files where an asset may be selected
  WAssetFileExtensionWhitelist::AddAssetFileExtension("CompatibleAsset_Texture_Cube", "dds");

  m_DocTypeDesc.m_sDocumentTypeName = "Texture Cube";
  m_DocTypeDesc.m_sFileExtension = "WTextureCubeAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/Texture_Cube.svg";
  m_DocTypeDesc.m_sAssetCategory = "Rendering";
  m_DocTypeDesc.m_pDocumentType = WGetStaticRTTI<WTextureCubeAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Texture_Cube");

  m_DocTypeDesc.m_sResourceFileExtension = "WBinTextureCube";
  m_DocTypeDesc.m_AssetDocumentFlags = WAssetDocumentFlags::AutoThumbnailOnTransform;
}

WTextureCubeAssetDocumentManager::~WTextureCubeAssetDocumentManager()
{
  WDocumentManager::s_Events.RemoveEventHandler(WMakeDelegate(&WTextureCubeAssetDocumentManager::OnDocumentManagerEvent, this));
}

void WTextureCubeAssetDocumentManager::OnDocumentManagerEvent(const WDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case WDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == WGetStaticRTTI<WTextureCubeAssetDocument>())
      {
        new WQtTextureCubeAssetDocumentWindow(static_cast<WTextureCubeAssetDocument*>(e.m_pDocument)); // NOLINT: Not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void WTextureCubeAssetDocumentManager::InternalCreateDocument(
  WStringView sDocumentTypeName, WStringView sPath, bool bCreateNewDocument, WDocument*& out_pDocument, const WDocumentObject* pOpenContext)
{
  out_pDocument = new WTextureCubeAssetDocument(sPath);
}

void WTextureCubeAssetDocumentManager::InternalGetSupportedDocumentTypes(WDynamicArray<const WDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}

WUInt64 WTextureCubeAssetDocumentManager::ComputeAssetProfileHashImpl(const WPlatformProfile* pAssetProfile) const
{
  // don't have any settings yet, but assets that generate profile specific output must not return 0 here
  return 1;
}

void WTextureCubeAssetDocumentManager::AppendAssetInfoSummary(WStringBuilder& ref_sOut, const WAssetInfoFile& info, WStringView sLinePrefix) const
{
  const WStringView keys[] = {WAssetInfoFile::Keys::ImageWidth, WAssetInfoFile::Keys::Format};
  info.AppendValuesToDisplayString(ref_sOut, keys, sLinePrefix);
}
