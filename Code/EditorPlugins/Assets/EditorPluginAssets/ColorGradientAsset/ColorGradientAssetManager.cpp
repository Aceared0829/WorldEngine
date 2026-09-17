#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/ColorGradientAsset/ColorGradientAsset.h>
#include <EditorPluginAssets/ColorGradientAsset/ColorGradientAssetManager.h>
#include <EditorPluginAssets/ColorGradientAsset/ColorGradientAssetWindow.moc.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WColorGradientAssetDocumentManager, 1, WRTTIDefaultAllocator<WColorGradientAssetDocumentManager>)
W_END_DYNAMIC_REFLECTED_TYPE;

WColorGradientAssetDocumentManager::WColorGradientAssetDocumentManager()
{
  WDocumentManager::s_Events.AddEventHandler(WMakeDelegate(&WColorGradientAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "ColorGradient";
  m_DocTypeDesc.m_sFileExtension = "WColorGradientAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/ColorGradient.svg";
  m_DocTypeDesc.m_sAssetCategory = "Animation";
  m_DocTypeDesc.m_pDocumentType = WGetStaticRTTI<WColorGradientAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Data_Gradient");
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Texture_2D"); // color gradients can be used as 2D textures

  m_DocTypeDesc.m_sResourceFileExtension = "WBinColorGradient";
  m_DocTypeDesc.m_AssetDocumentFlags = WAssetDocumentFlags::AutoTransformOnSave | WAssetDocumentFlags::SupportsThumbnail;
}

WColorGradientAssetDocumentManager::~WColorGradientAssetDocumentManager()
{
  WDocumentManager::s_Events.RemoveEventHandler(WMakeDelegate(&WColorGradientAssetDocumentManager::OnDocumentManagerEvent, this));
}

void WColorGradientAssetDocumentManager::OnDocumentManagerEvent(const WDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case WDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == WGetStaticRTTI<WColorGradientAssetDocument>())
      {
        new WQtColorGradientAssetDocumentWindow(e.m_pDocument); // NOLINT: not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void WColorGradientAssetDocumentManager::InternalCreateDocument(
  WStringView sDocumentTypeName, WStringView sPath, bool bCreateNewDocument, WDocument*& out_pDocument, const WDocumentObject* pOpenContext)
{
  out_pDocument = new WColorGradientAssetDocument(sPath);
}

void WColorGradientAssetDocumentManager::InternalGetSupportedDocumentTypes(
  WDynamicArray<const WDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
