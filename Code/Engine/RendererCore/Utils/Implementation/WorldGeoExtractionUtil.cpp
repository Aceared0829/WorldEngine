#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/World/World.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Math/Mat3.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <RendererCore/Meshes/CpuMeshResource.h>
#include <RendererCore/Meshes/MeshBufferUtils.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>

// clang-format off
W_IMPLEMENT_MESSAGE_TYPE(WMsgExtractGeometry);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMsgExtractGeometry, 1, WRTTIDefaultAllocator<WMsgExtractGeometry>)
{
  W_BEGIN_ATTRIBUTES
  {
    new WExcludeFromScript()
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void WWorldGeoExtractionUtil::ExtractWorldGeometry(MeshObjectList& ref_objects, const WWorld& world, ExtractionMode mode, WTagSet* pExcludeTags /*= nullptr*/)
{
  W_PROFILE_SCOPE("ExtractWorldGeometry");
  W_LOG_BLOCK("ExtractWorldGeometry", world.GetName());

  WMsgExtractGeometry msg;
  msg.m_Mode = mode;
  msg.m_pMeshObjects = &ref_objects;

  W_LOCK(world.GetReadMarker());

  for (auto it = world.GetObjects(); it.IsValid(); ++it)
  {
    if (pExcludeTags != nullptr && it->GetTags().IsAnySet(*pExcludeTags))
      continue;

    it->SendMessage(msg);
  }
}

void WWorldGeoExtractionUtil::ExtractWorldGeometry(MeshObjectList& ref_objects, const WWorld& world, ExtractionMode mode, const WDeque<WGameObjectHandle>& selection)
{
  W_PROFILE_SCOPE("ExtractWorldGeometry");
  W_LOG_BLOCK("ExtractWorldGeometry", world.GetName());

  WMsgExtractGeometry msg;
  msg.m_Mode = mode;
  msg.m_pMeshObjects = &ref_objects;

  W_LOCK(world.GetReadMarker());

  for (WGameObjectHandle hObject : selection)
  {
    const WGameObject* pObject;
    if (!world.TryGetObject(hObject, pObject))
      continue;

    pObject->SendMessage(msg);
  }
}

void WWorldGeoExtractionUtil::WriteWorldGeometryToOBJ(const char* szFile, const MeshObjectList& objects, const WMat3& mTransform)
{
  W_LOG_BLOCK("Write World Geometry to OBJ", szFile);

  WFileWriter file;
  if (file.Open(szFile).Failed())
  {
    WLog::Error("Failed to open file for writing: '{0}'", szFile);
    return;
  }

  WMat4 transform = WMat4::MakeIdentity();
  transform.SetRotationalPart(mTransform);

  WStringBuilder line;

  line = "\n\n# vertices\n\n";
  file.WriteBytes(line.GetData(), line.GetElementCount()).IgnoreResult();

  WUInt32 uiVertexOffset = 0;
  WDeque<WUInt32> indices;

  for (const MeshObject& object : objects)
  {
    WResourceLock<WCpuMeshResource> pCpuMesh(object.m_hMeshResource, WResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (pCpuMesh.GetAcquireResult() != WResourceAcquireResult::Final)
    {
      continue;
    }

    const auto& meshBufferDesc = pCpuMesh->GetDescriptor().MeshBufferDesc();

    const WVec3* pPositions = meshBufferDesc.GetPositionData().GetPtr();
    if (pPositions == nullptr)
    {
      continue;
    }

    WMat4 finalTransform = transform * object.m_GlobalTransform.GetAsMat4();

    // write out all vertices
    for (WUInt32 i = 0; i < meshBufferDesc.GetVertexCount(); ++i)
    {
      const WVec3 pos = finalTransform.TransformPosition(*pPositions);

      line.SetFormat("v {0} {1} {2}\n", WArgF(pos.x, 8), WArgF(pos.y, 8), WArgF(pos.z, 8));
      file.WriteBytes(line.GetData(), line.GetElementCount()).IgnoreResult();

      ++pPositions;
    }

    // collect all indices
    bool flip = WGraphicsUtils::IsTriangleFlipRequired(finalTransform.GetRotationalPart());

    if (meshBufferDesc.HasIndexBuffer())
    {
      if (meshBufferDesc.Uses32BitIndices())
      {
        const WUInt32* pTypedIndices = reinterpret_cast<const WUInt32*>(meshBufferDesc.GetIndexBufferData().GetPtr());

        for (WUInt32 p = 0; p < meshBufferDesc.GetPrimitiveCount(); ++p)
        {
          indices.PushBack(pTypedIndices[p * 3 + (flip ? 2 : 0)] + uiVertexOffset);
          indices.PushBack(pTypedIndices[p * 3 + 1] + uiVertexOffset);
          indices.PushBack(pTypedIndices[p * 3 + (flip ? 0 : 2)] + uiVertexOffset);
        }
      }
      else
      {
        const WUInt16* pTypedIndices = reinterpret_cast<const WUInt16*>(meshBufferDesc.GetIndexBufferData().GetPtr());

        for (WUInt32 p = 0; p < meshBufferDesc.GetPrimitiveCount(); ++p)
        {
          indices.PushBack(pTypedIndices[p * 3 + (flip ? 2 : 0)] + uiVertexOffset);
          indices.PushBack(pTypedIndices[p * 3 + 1] + uiVertexOffset);
          indices.PushBack(pTypedIndices[p * 3 + (flip ? 0 : 2)] + uiVertexOffset);
        }
      }
    }
    else
    {
      for (WUInt32 v = 0; v < meshBufferDesc.GetVertexCount(); ++v)
      {
        indices.PushBack(uiVertexOffset + v);
      }
    }

    uiVertexOffset += meshBufferDesc.GetVertexCount();
  }

  line = "\n\n# triangles\n\n";
  file.WriteBytes(line.GetData(), line.GetElementCount()).IgnoreResult();

  for (WUInt32 i = 0; i < indices.GetCount(); i += 3)
  {
    // indices are 1 based in obj
    line.SetFormat("f {0} {1} {2}\n", indices[i + 0] + 1, indices[i + 1] + 1, indices[i + 2] + 1);
    file.WriteBytes(line.GetData(), line.GetElementCount()).IgnoreResult();
  }

  WLog::Success("Wrote world geometry to '{0}'", file.GetFilePathAbsolute().GetView());
}

//////////////////////////////////////////////////////////////////////////

void WMsgExtractGeometry::AddMeshObject(const WTransform& transform, WCpuMeshResourceHandle hMeshResource)
{
  m_pMeshObjects->PushBack({transform, hMeshResource});
}

void WMsgExtractGeometry::AddBox(const WTransform& transform, WVec3 vExtents)
{
  const char* szResourceName = "CpuMesh-UnitBox";
  WCpuMeshResourceHandle hBoxMesh = WResourceManager::GetExistingResource<WCpuMeshResource>(szResourceName);
  if (hBoxMesh.IsValid() == false)
  {
    WGeometry geom;
    geom.AddBox(WVec3(1), false);
    geom.TriangulatePolygons();
    geom.ComputeTangents();

    WMeshResourceDescriptor desc;
    desc.SetMaterial(0, "{ 1c47ee4c-0379-4280-85f5-b8cda61941d2 }"); // Data/Base/Materials/Common/Pattern.WMaterialAsset

    desc.MeshBufferDesc().AddCommonStreams();
    desc.MeshBufferDesc().AllocateStreamsFromGeometry(geom, WGALPrimitiveTopology::Triangles);

    desc.AddSubMesh(desc.MeshBufferDesc().GetPrimitiveCount(), 0, 0);

    desc.ComputeBounds();

    hBoxMesh = WResourceManager::GetOrCreateResource<WCpuMeshResource>(szResourceName, std::move(desc), szResourceName);
  }

  auto& meshObject = m_pMeshObjects->ExpandAndGetRef();
  meshObject.m_GlobalTransform = transform;
  meshObject.m_GlobalTransform.m_vScale *= vExtents;
  meshObject.m_hMeshResource = hBoxMesh;
}

W_STATICLINK_FILE(RendererCore, RendererCore_Utils_Implementation_WorldGeoExtractionUtil);
