#pragma once

#include <JoltPlugin/JoltPluginDLL.h>
#include <JoltPlugin/Resources/JoltMeshResource.h>

#include <Core/World/World.h>

struct WMsgGenerateSplineMeshCollision;
struct WMsgComponentInternalTrigger;
class WAbstractObjectNode;
using WMeshResourceHandle = WTypedResourceHandle<class WMeshResource>;

struct W_JOLTPLUGIN_DLL WJoltMeshMapping
{
  WMeshResourceHandle m_hRenderMesh;
  WJoltMeshResourceHandle m_hCollisionMesh;

  bool operator==(const WJoltMeshMapping& other) const
  {
    return m_hRenderMesh == other.m_hRenderMesh && m_hCollisionMesh == other.m_hCollisionMesh;
  }

  WResult Serialize(WStreamWriter& inout_stream) const;
  WResult Deserialize(WStreamReader& inout_stream);
};

W_DECLARE_REFLECTABLE_TYPE(W_JOLTPLUGIN_DLL, WJoltMeshMapping);

//////////////////////////////////////////////////////////////////////////

using WJoltGenerateCollisionComponentManager = WComponentManager<class WJoltGenerateCollisionComponent, WBlockStorageType::Compact>;

/// A component that generates a static collision mesh from specified render meshes.
///
/// The generated collision mesh is written to disk as a JoltMeshResource. During scene export the
/// WSceneExportModifier_JoltFinalizeGeneratedCollision export modifier creates a static actor which references the generated collision mesh.
/// This component has no effect at runtime and is removed from scenes (not prefabs though) by the export modifier.
/// Currently only supports generation from spline meshes.
class W_JOLTPLUGIN_DLL WJoltGenerateCollisionComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(WJoltGenerateCollisionComponent, WComponent, WJoltGenerateCollisionComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

protected:
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // WJoltGenerateCollisionComponent

public:
  WJoltGenerateCollisionComponent();
  ~WJoltGenerateCollisionComponent();

  /// Mesh mappings define which jolt collision mesh should be used for a corresponding render mesh.
  /// Render meshes without a mapping won't have collision.
  WArrayPtr<const WJoltMeshMapping> GetMeshMappings() const { return m_MeshMappings; }                         // [ property ]

  WUInt8 m_uiCollisionLayer = 0;                                                                                // [ property ]

private:
  WUInt32 Reflection_GetMeshMappingCount() const { return m_MeshMappings.GetCount(); }                          // [ property ]
  const WJoltMeshMapping& Reflection_GetMeshMapping(WUInt32 uiIndex) const { return m_MeshMappings[uiIndex]; } // [ property ]
  void Reflection_SetMeshMapping(WUInt32 uiIndex, const WJoltMeshMapping& mapping);                            // [ property ]
  void Reflection_InsertMeshMapping(WUInt32 uiIndex, const WJoltMeshMapping& mapping);                         // [ property ]
  void Reflection_RemoveMeshMapping(WUInt32 uiIndex);                                                           // [ property ]

  WCpuMeshResourceHandle GetCollisionCpuMeshForRenderMesh(WMeshResourceHandle hRenderMesh) const;

  void OnObjectCreated(const WAbstractObjectNode& node);
  void OnMsgGenerateSplineMeshCollision(WMsgGenerateSplineMeshCollision& ref_msg); // [ msg handler ]
  void OnMsgComponentInternalTrigger(WMsgComponentInternalTrigger& ref_msg);       // [ msg handler ]

  void StartGenerateTask(WSharedPtr<WTask>&& pTask);

  friend class WSceneExportModifier_JoltFinalizeGeneratedCollision;
  void FinalizeGeneration();

  WSmallArray<WJoltMeshMapping, 1> m_MeshMappings;

  WUInt64 m_uiStableId = 0;
  WHashedString m_sCollisionMeshPath;

  WSharedPtr<WTask> m_pGenerationTask;
  WSharedPtr<WTask> m_pNextGenerationTask;
  WTaskGroupID m_TaskGroupID;
};
