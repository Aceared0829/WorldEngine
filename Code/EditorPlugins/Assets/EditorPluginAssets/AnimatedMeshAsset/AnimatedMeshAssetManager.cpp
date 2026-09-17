#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/AnimatedMeshAsset/AnimatedMeshAssetManager.h>
#include <EditorPluginAssets/AnimatedMeshAsset/AnimatedMeshAssetWindow.moc.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WAnimatedMeshAssetDocumentManager, 1, WRTTIDefaultAllocator<WAnimatedMeshAssetDocumentManager>)
W_END_DYNAMIC_REFLECTED_TYPE;

WAnimatedMeshAssetDocumentManager::WAnimatedMeshAssetDocumentManager()
{
  WDocumentManager::s_Events.AddEventHandler(WMakeDelegate(&WAnimatedMeshAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "Animated Mesh";
  m_DocTypeDesc.m_sFileExtension = "WAnimatedMeshAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/Animated_Mesh.svg";
  m_DocTypeDesc.m_sAssetCategory = "Rendering";
  m_DocTypeDesc.m_pDocumentType = WGetStaticRTTI<WAnimatedMeshAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Mesh_Static");
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Mesh_Skinned");

  m_DocTypeDesc.m_AssetDocumentFlags = WAssetDocumentFlags::SupportsThumbnail;
  m_DocTypeDesc.m_sResourceFileExtension = "WBinAnimatedMesh";
}

WAnimatedMeshAssetDocumentManager::~WAnimatedMeshAssetDocumentManager()
{
  WDocumentManager::s_Events.RemoveEventHandler(WMakeDelegate(&WAnimatedMeshAssetDocumentManager::OnDocumentManagerEvent, this));
}

void WAnimatedMeshAssetDocumentManager::OnDocumentManagerEvent(const WDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case WDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == WGetStaticRTTI<WAnimatedMeshAssetDocument>())
      {
        new WQtAnimatedMeshAssetDocumentWindow(static_cast<WAnimatedMeshAssetDocument*>(e.m_pDocument)); // NOLINT
      }
    }
    break;

    default:
      break;
  }
}

void WAnimatedMeshAssetDocumentManager::InternalCreateDocument(WStringView sDocumentTypeName, WStringView sPath, bool bCreateNewDocument, WDocument*& out_pDocument, const WDocumentObject* pOpenContext)
{
  out_pDocument = new WAnimatedMeshAssetDocument(sPath);
}

void WAnimatedMeshAssetDocumentManager::InternalGetSupportedDocumentTypes(WDynamicArray<const WDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}

void WAnimatedMeshAssetDocumentManager::AppendAssetInfoSummary(WStringBuilder& ref_sOut, const WAssetInfoFile& info, WStringView sLinePrefix) const
{
  const WStringView keys[] = {WAssetInfoFile::Keys::NumTriangles, WAssetInfoFile::Keys::BoundsHalfExtents};
  info.AppendValuesToDisplayString(ref_sOut, keys, sLinePrefix);
}
