#include <EditorPluginJolt/EditorPluginJoltPCH.h>

#include <EditorPluginJolt/CollisionMeshAsset/JoltCollisionMeshAssetManager.h>
#include <EditorPluginJolt/CollisionMeshAsset/JoltCollisionMeshAssetWindow.moc.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WJoltCollisionMeshAssetDocumentManager, 1, WRTTIDefaultAllocator<WJoltCollisionMeshAssetDocumentManager>)
W_END_DYNAMIC_REFLECTED_TYPE;

WJoltCollisionMeshAssetDocumentManager::WJoltCollisionMeshAssetDocumentManager()
{
  WDocumentManager::s_Events.AddEventHandler(WMakeDelegate(&WJoltCollisionMeshAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "Jolt_Colmesh_Triangle";
  m_DocTypeDesc.m_sFileExtension = "WJoltCollisionMeshAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/Jolt_Collision_Mesh.svg";
  m_DocTypeDesc.m_sAssetCategory = "Physics";
  m_DocTypeDesc.m_pDocumentType = WGetStaticRTTI<WJoltCollisionMeshAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Jolt_Colmesh_Triangle");

  m_DocTypeDesc.m_sResourceFileExtension = "WBinJoltTriangleMesh";
  m_DocTypeDesc.m_AssetDocumentFlags = WAssetDocumentFlags::SupportsThumbnail;

  m_DocTypeDesc2.m_sDocumentTypeName = "Jolt_Colmesh_Convex";
  m_DocTypeDesc2.m_sFileExtension = "WJoltConvexCollisionMeshAsset";
  m_DocTypeDesc2.m_sIcon = ":/AssetIcons/Jolt_Collision_Mesh_Convex.svg";
  m_DocTypeDesc2.m_sAssetCategory = "Physics";
  m_DocTypeDesc2.m_pDocumentType = WGetStaticRTTI<WJoltCollisionMeshAssetDocument>();
  m_DocTypeDesc2.m_pManager = this;
  m_DocTypeDesc2.m_CompatibleTypes.PushBack("CompatibleAsset_Jolt_Colmesh_Triangle"); // convex meshes can also be used as triangle meshes (concave)
  m_DocTypeDesc2.m_CompatibleTypes.PushBack("CompatibleAsset_Jolt_Colmesh_Convex");

  m_DocTypeDesc2.m_sResourceFileExtension = "WBinJoltConvexMesh";
  m_DocTypeDesc2.m_AssetDocumentFlags = WAssetDocumentFlags::SupportsThumbnail;
}

WJoltCollisionMeshAssetDocumentManager::~WJoltCollisionMeshAssetDocumentManager()
{
  WDocumentManager::s_Events.RemoveEventHandler(WMakeDelegate(&WJoltCollisionMeshAssetDocumentManager::OnDocumentManagerEvent, this));
}

void WJoltCollisionMeshAssetDocumentManager::OnDocumentManagerEvent(const WDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case WDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == WGetStaticRTTI<WJoltCollisionMeshAssetDocument>())
      {
        new WQtJoltCollisionMeshAssetDocumentWindow(static_cast<WAssetDocument*>(e.m_pDocument)); // NOLINT: Not a memory leak
      }
    }
    break;
    default:
      break;
  }
}

void WJoltCollisionMeshAssetDocumentManager::InternalCreateDocument(WStringView sDocumentTypeName, WStringView sPath, bool bCreateNewDocument, WDocument*& out_pDocument, const WDocumentObject* pOpenContext)
{
  if (sDocumentTypeName.IsEqual("Jolt_Colmesh_Convex"))
  {
    out_pDocument = new WJoltCollisionMeshAssetDocument(sPath, true);
  }
  else
  {
    out_pDocument = new WJoltCollisionMeshAssetDocument(sPath, false);
  }
}

void WJoltCollisionMeshAssetDocumentManager::InternalGetSupportedDocumentTypes(WDynamicArray<const WDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
  inout_DocumentTypes.PushBack(&m_DocTypeDesc2);
}

WUInt64 WJoltCollisionMeshAssetDocumentManager::ComputeAssetProfileHashImpl(const WPlatformProfile* pAssetProfile) const
{
  // don't have any settings yet, but assets that generate profile specific output must not return 0 here
  return 1;
}

void WJoltCollisionMeshAssetDocumentManager::AppendAssetInfoSummary(WStringBuilder& ref_sOut, const WAssetInfoFile& info, WStringView sLinePrefix) const
{
  const WStringView keys[] = {WAssetInfoFile::Keys::CollisionMeshType, WAssetInfoFile::Keys::NumConvexParts, WAssetInfoFile::Keys::NumTriangles};
  info.AppendValuesToDisplayString(ref_sOut, keys, sLinePrefix);
}
