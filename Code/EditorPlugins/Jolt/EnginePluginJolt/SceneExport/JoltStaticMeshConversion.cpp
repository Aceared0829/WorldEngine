#include <EnginePluginJolt/EnginePluginJoltPCH.h>

#include <EnginePluginJolt/SceneExport/JoltStaticMeshConversion.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <JoltPlugin/Actors/JoltStaticActorComponent.h>
#include <JoltPlugin/Resources/JoltMeshResourceWriter.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSceneExportModifier_JoltStaticMeshConversion, 1, WRTTIDefaultAllocator<WSceneExportModifier_JoltStaticMeshConversion>)
W_END_DYNAMIC_REFLECTED_TYPE;

void WSceneExportModifier_JoltStaticMeshConversion::ModifyWorld(WWorld& ref_world, WStringView sDocumentType, const WUuid& documentGuid, bool bForExport)
{
  if (sDocumentType == "Prefab")
  {
    // the auto generated static meshes are needed in the prefab document, so that physical interactions for previewing purposes work
    // however, the scene also exports the static colmesh, including all the prefabs (with overridden materials)
    // in the final scene this would create double colmeshes in the same place, but the materials may differ
    // therefore we don't want to export the colmesh other than for preview purposes, so we ignore this, if 'bForExport' is true

    if (bForExport)
    {
      return;
    }
  }

  W_LOCK(ref_world.GetWriteMarker());

  WSmcDescription desc;
  desc.m_Surfaces.PushBack(); // add a dummy empty material

  WMsgBuildStaticMesh msg;
  msg.m_pStaticMeshDescription = &desc;

  for (auto it = ref_world.GetObjects(); it.IsValid(); ++it)
  {
    if (!it->IsStatic())
      continue;

    it->SendMessage(msg);
  }

  if (desc.m_SubMeshes.IsEmpty() || desc.m_Vertices.IsEmpty() || desc.m_Triangles.IsEmpty())
    return;

  const WUInt32 uiNumVertices = desc.m_Vertices.GetCount();
  const WUInt32 uiNumTriangles = desc.m_Triangles.GetCount();
  const WUInt32 uiNumSubMeshes = desc.m_SubMeshes.GetCount();

  WJoltMeshDesc meshDesc;
  meshDesc.m_Type = WJoltMeshDesc::Type::Triangle;
  meshDesc.m_Vertices.SetCountUninitialized(uiNumVertices);

  for (WUInt32 i = 0; i < uiNumVertices; ++i)
  {
    meshDesc.m_Vertices[i] = desc.m_Vertices[i];
  }

  meshDesc.m_TriangleIndices.SetCountUninitialized(uiNumTriangles * 3);
  meshDesc.m_TriangleSurfaceID.SetCount(uiNumTriangles);

  for (WUInt32 i = 0; i < uiNumTriangles; ++i)
  {
    meshDesc.m_TriangleIndices[i * 3 + 0] = desc.m_Triangles[i].m_uiVertexIndices[0];
    meshDesc.m_TriangleIndices[i * 3 + 1] = desc.m_Triangles[i].m_uiVertexIndices[1];
    meshDesc.m_TriangleIndices[i * 3 + 2] = desc.m_Triangles[i].m_uiVertexIndices[2];
  }

  // copy materials
  // we could collate identical materials here and merge meshes, but the mesh cooking will probably do the same already
  {
    for (WUInt32 i = 0; i < desc.m_Surfaces.GetCount(); ++i)
    {
      meshDesc.m_Surfaces.PushBack(desc.m_Surfaces[i]);
    }

    for (WUInt32 i = 0; i < uiNumSubMeshes; ++i)
    {
      const WUInt32 uiLastTriangle = desc.m_SubMeshes[i].m_uiFirstTriangle + desc.m_SubMeshes[i].m_uiNumTriangles;
      const WUInt16 uiSurface = desc.m_SubMeshes[i].m_uiSurfaceIndex;

      for (WUInt32 t = desc.m_SubMeshes[i].m_uiFirstTriangle; t < uiLastTriangle; ++t)
      {
        meshDesc.m_TriangleSurfaceID[t] = uiSurface;
      }
    }
  }

  WStringBuilder sDocGuid, sOutputFile;
  WConversionUtils::ToString(documentGuid, sDocGuid);

  sOutputFile.SetFormat(":project/AssetCache/Generated/{0}.WJoltMesh", sDocGuid);

  WDeferredFileWriter file;
  file.SetOutput(sOutputFile);

  if (WJoltMeshResourceWriter::WriteMeshResource(std::move(meshDesc), file).Failed())
  {
    WLog::Error("Could not write to global collision mesh file");
    return;
  }

  if (file.Close().Failed())
  {
    WLog::Error("Could not write to global collision mesh file");
    return;
  }

  {
    WGameObject* pGo;
    WGameObjectDesc god;
    god.m_sName.Assign("Greybox Collision Mesh");
    ref_world.CreateObject(god, pGo);

    auto* pCompMan = ref_world.GetOrCreateComponentManager<WJoltStaticActorComponentManager>();

    WJoltStaticActorComponent* pComp;
    pCompMan->CreateComponent(pGo, pComp);

    if (!sOutputFile.IsEmpty())
    {
      WJoltMeshResourceHandle hMesh = WResourceManager::LoadResource<WJoltMeshResource>(sOutputFile);
      pComp->SetMesh(hMesh);
    }
  }
}
