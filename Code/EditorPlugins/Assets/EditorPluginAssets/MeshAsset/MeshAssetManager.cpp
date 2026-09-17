#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginAssets/MeshAsset/MeshAssetManager.h>
#include <EditorPluginAssets/MeshAsset/MeshAssetWindow.moc.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <ToolsFoundation/Assets/AssetFileExtensionWhitelist.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMeshAssetDocumentManager, 1, WRTTIDefaultAllocator<WMeshAssetDocumentManager>)
W_END_DYNAMIC_REFLECTED_TYPE;

WMeshAssetDocumentManager::WMeshAssetDocumentManager()
{
  WDocumentManager::s_Events.AddEventHandler(WMakeDelegate(&WMeshAssetDocumentManager::OnDocumentManagerEvent, this));

  // additional whitelist for non-asset files where an asset may be selected
  WAssetFileExtensionWhitelist::AddAssetFileExtension("CompatibleAsset_Mesh_Static", "WBinMesh");

  m_DocTypeDesc.m_sDocumentTypeName = "Mesh";
  m_DocTypeDesc.m_sFileExtension = "WMeshAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/Mesh.svg";
  m_DocTypeDesc.m_sAssetCategory = "Rendering";
  m_DocTypeDesc.m_pDocumentType = WGetStaticRTTI<WMeshAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Mesh_Static");

  m_DocTypeDesc.m_sResourceFileExtension = "WBinMesh";
  m_DocTypeDesc.m_AssetDocumentFlags = WAssetDocumentFlags::SupportsThumbnail;
}

WMeshAssetDocumentManager::~WMeshAssetDocumentManager()
{
  WDocumentManager::s_Events.RemoveEventHandler(WMakeDelegate(&WMeshAssetDocumentManager::OnDocumentManagerEvent, this));
}

WResult WMeshAssetDocumentManager::OpenPickedDocument(const WDocumentObject* pPickedComponent, WUInt32 uiPartIndex)
{
  // check that we actually picked a mesh component
  if (!pPickedComponent->GetTypeAccessor().GetType()->IsDerivedFrom<WMeshComponent>())
    return W_FAILURE;

  // first try the materials array on the component itself, and see if we have a material override to pick
  if ((WInt32)uiPartIndex < pPickedComponent->GetTypeAccessor().GetCount("Materials"))
  {
    // access the material at the given index
    // this might be empty, though, in which case we still need to check the mesh asset
    const WVariant varMatGuid = pPickedComponent->GetTypeAccessor().GetValue("Materials", uiPartIndex);

    // if it were anything else than a string that would be weird
    W_ASSERT_DEV(varMatGuid.IsA<WString>(), "Material override property is not a string type");

    if (varMatGuid.IsA<WString>())
    {
      if (TryOpenAssetDocument(varMatGuid.Get<WString>()).Succeeded())
        return W_SUCCESS;
    }
  }

  // couldn't open it through the override, so we now need to inspect the mesh asset
  const WVariant varMeshGuid = pPickedComponent->GetTypeAccessor().GetValue("Mesh");

  W_ASSERT_DEV(varMeshGuid.IsA<WString>(), "Mesh property is not a string type");

  if (!varMeshGuid.IsA<WString>())
    return W_FAILURE;

  // we don't support non-guid mesh asset references, because I'm too lazy
  if (!WConversionUtils::IsStringUuid(varMeshGuid.Get<WString>()))
    return W_FAILURE;

  const WUuid meshGuid = WConversionUtils::ConvertStringToUuid(varMeshGuid.Get<WString>());

  auto pSubAsset = WAssetCurator::GetSingleton()->GetSubAsset(meshGuid);

  // unknown mesh asset
  if (!pSubAsset)
    return W_FAILURE;

  // now we need to open the mesh and we cannot wait for it (usually that is queued for GUI reasons)
  // though we do not want a window
  WMeshAssetDocument* pMeshDoc =
    static_cast<WMeshAssetDocument*>(WQtEditorApp::GetSingleton()->OpenDocument(pSubAsset->m_pAssetInfo->m_Path.GetAbsolutePath(), WDocumentFlags::None));

  if (!pMeshDoc)
    return W_FAILURE;

  WResult result = W_FAILURE;

  // if we are outside the stored index, tough luck
  if (uiPartIndex < pMeshDoc->GetProperties()->m_Slots.GetCount())
  {
    result = TryOpenAssetDocument(pMeshDoc->GetProperties()->m_Slots[uiPartIndex].m_sResource);
  }

  // make sure to close the document again, if we were the ones to open it
  // otherwise keep it open
  if (!pMeshDoc->HasWindowBeenRequested())
    pMeshDoc->GetDocumentManager()->CloseDocument(pMeshDoc);

  return result;
}

void WMeshAssetDocumentManager::OnDocumentManagerEvent(const WDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case WDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == WGetStaticRTTI<WMeshAssetDocument>())
      {
        new WQtMeshAssetDocumentWindow(static_cast<WMeshAssetDocument*>(e.m_pDocument)); // NOLINT: Not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void WMeshAssetDocumentManager::InternalCreateDocument(
  WStringView sDocumentTypeName, WStringView sPath, bool bCreateNewDocument, WDocument*& out_pDocument, const WDocumentObject* pOpenContext)
{
  out_pDocument = new WMeshAssetDocument(sPath);
}

void WMeshAssetDocumentManager::InternalGetSupportedDocumentTypes(WDynamicArray<const WDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}

void WMeshAssetDocumentManager::AppendAssetInfoSummary(WStringBuilder& ref_sOut, const WAssetInfoFile& info, WStringView sLinePrefix) const
{
  const WStringView keys[] = {WAssetInfoFile::Keys::NumTriangles, WAssetInfoFile::Keys::BoundsHalfExtents};
  info.AppendValuesToDisplayString(ref_sOut, keys, sLinePrefix);
}
