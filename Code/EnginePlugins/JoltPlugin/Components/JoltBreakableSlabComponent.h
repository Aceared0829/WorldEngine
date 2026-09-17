#pragma once

#include <JoltPlugin/JoltPluginDLL.h>

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Threading/TaskSystem.h>
#include <GameEngine/Physics/Breakable2D.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Meshes/SkinnedMeshRenderData.h>

#include <Jolt/Jolt.h>

#include <Jolt/Core/Reference.h>

using WMaterialResourceHandle = WTypedResourceHandle<class WMaterialResource>;
using WMeshResourceHandle = WTypedResourceHandle<class WMeshResource>;

struct WMsgPhysicsAddImpulse;
struct WMsgExtractRenderData;
struct WMsgPhysicContact;
struct WMsgPhysicCharacterContact;
class WGeometry;
class WMeshResourceDescriptor;
class WJoltMaterial;

namespace JPH
{
  class BodyInterface;
  class ConvexShape;
} // namespace JPH

/// Flags that define which edges of a breakable slab are fixed/anchored in the world.
/// A fixed edge means that shards adjacent to that edge will remain stationary and not fall under gravity.
struct W_JOLTPLUGIN_DLL WJoltBreakableSlabFlags
{
  using StorageType = WUInt8;

  enum Enum
  {
    FixedEdgeTop = W_BIT(0),    ///< Shards will stick to this edge
    FixedEdgeRight = W_BIT(1),  ///< Shards will stick to this edge
    FixedEdgeBottom = W_BIT(2), ///< Shards will stick to this edge
    FixedEdgeLeft = W_BIT(3),   ///< Shards will stick to this edge

    Default = FixedEdgeTop | FixedEdgeRight | FixedEdgeBottom | FixedEdgeLeft
  };

  struct Bits
  {
    StorageType FixedEdgeTop : 1;
    StorageType FixedEdgeRight : 1;
    StorageType FixedEdgeBottom : 1;
    StorageType FixedEdgeLeft : 1;
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_JOLTPLUGIN_DLL, WJoltBreakableSlabFlags);

/// The general shape of the breakable slab.
///
/// Can be extended with further shapes, if desired.
struct W_JOLTPLUGIN_DLL WJoltBreakableShape
{
  using StorageType = WUInt8;

  enum Enum
  {
    Rectangle,
    Triangle,
    Circle,

    Default = Rectangle
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_JOLTPLUGIN_DLL, WJoltBreakableShape);

class W_JOLTPLUGIN_DLL WJoltBreakableSlabComponentManager : public WComponentManager<class WJoltBreakableSlabComponent, WBlockStorageType::FreeList>
{
public:
  WJoltBreakableSlabComponentManager(WWorld* pWorld);
  ~WJoltBreakableSlabComponentManager();

  virtual void Initialize() override;

private:
  friend class WJoltWorldModule;
  friend class WJoltBreakableSlabComponent;

  void PreAsyncUpdate(const WWorldModule::UpdateContext& context);
  void ReinitSlabs(const WWorldModule::UpdateContext& context);
  void PostAsyncUpdate(const WWorldModule::UpdateContext& context);

  WSet<WComponentHandle> m_RequireBreakage;
  WSet<WComponentHandle> m_RequireBreakUpdate;

  WAtomicInteger32 m_iTriggerBoundsUpdateSlot;
  WDynamicArray<WJoltBreakableSlabComponent*> m_TriggerBoundsUpdate;
};

/// Represents a point in world space, where the breakable slab should be shattered.
struct WShatterPoint
{
  WVec3 m_vGlobalPosition;
  WVec3 m_vImpulse;                                                        ///< With which impulse to push aways new, dynamic shards.
  WUInt32 m_uiShardIdx = WInvalidIndex;                                   ///< Which shard to break apart.
  float m_fImpactRadius = 0.05f;                                            ///< The size of the shatter point. Only relevant for some patterns.
  float m_fCellSize = 0.4f;                                                 ///< For the cellular (voronoi) pattern, how large to make cells.
  float m_fMakeDynamicRadius = 0.25f;                                       ///< In what radius around the shatter position to always make new shards dynamic. Unsupported shards will become dynamic regardless.
  WUInt8 m_uiAllowedBreakPatterns = (WUInt8)WBreakablePattern::Cellular; ///< With which pattern to potentially break the shard.
};

/// Most of the shatter calculation (and physics collider generation) is done in this task, to prevent performance drops.
///
/// The result may be ready only with 1-3 frames delay.
class WShatterTask : public WTask
{
public:
  WJoltBreakableSlabComponent* m_pComponent = nullptr;
  WHybridArray<JPH::Ref<JPH::ConvexShape>, 128> m_Shapes;
  WMeshResourceHandle m_hShardsMesh;
  WHybridArray<WShatterPoint, 8> m_ShatterPoints;
  WVec3 m_vFinalImpulse;

protected:
  virtual void Execute() override;
};


/// Component that represents a destructible slab that can be broken into shards.
///
/// This component creates a breakable surface that can shatter into smaller pieces when hit.
/// The slab can be rectangular, triangular or circular and can be anchored on any of its edges.
/// When broken, the shards become dynamic physics objects that can collide and fall under gravity.
class W_JOLTPLUGIN_DLL WJoltBreakableSlabComponent : public WRenderComponent
{
  W_DECLARE_COMPONENT_TYPE(WJoltBreakableSlabComponent, WRenderComponent, WJoltBreakableSlabComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;
  virtual void OnSimulationStarted() override;


  //////////////////////////////////////////////////////////////////////////
  // WRenderComponent

public:
  virtual WResult GetLocalBounds(WBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, WMsgUpdateLocalBounds& ref_msg) override;

  //////////////////////////////////////////////////////////////////////////
  // WJoltBreakableSlabComponent

public:
  void SetWidth(float fWidth);         // [ property ]
  float GetWidth() const;              // [ property ]

  void SetHeight(float fHeight);       // [ property ]
  float GetHeight() const;             // [ property ]

  void SetThickness(float fThickness); // [ property ]
  float GetThickness() const;          // [ property ]

  /// Sets the UV scaling factor for texture mapping
  void SetUvScale(WVec2 vScale); // [ property ]
  WVec2 GetUvScale() const;      // [ property ]

  /// Sets which edges of the slab are fixed/anchored in the world
  void SetFlags(WBitflags<WJoltBreakableSlabFlags> flags); // [ property ]
  WBitflags<WJoltBreakableSlabFlags> GetFlags() const      // [ property ]
  {
    return m_Flags;
  }

  /// Sets the basic shape of the breakable slab (rectangle, triangle, circle)
  void SetShape(WEnum<WJoltBreakableShape> shape); // [ property ]
  WEnum<WJoltBreakableShape> GetShape() const      // [ property ]
  {
    return m_Shape;
  }

  /// Restores the slab to its original unbroken state
  void Restore(); // [ scriptable ]

  /// Shatters the entire slab into pieces of roughly the given size
  /// \param fShardSize The approximate size of generated shards
  /// \param vImpulse The impulse to apply to the shards
  void ShatterAll(float fShardSize, const WVec3& vImpulse); // [ scriptable ]

  /// Shatters the slab using a cellular (Voronoi) pattern around a point
  /// \param vGlobalPosition The world position where to initiate the break
  /// \param fCellSize The approximate size of the cellular shards
  /// \param vImpulse The impulse to apply to the shards
  /// \param fMakeDynamicRadius Radius around the break point where shards become dynamic
  void ShatterCellular(const WVec3& vGlobalPosition, float fCellSize, const WVec3& vImpulse, float fMakeDynamicRadius); // [ scriptable ]

  /// Shatters the slab in a radial pattern around a point
  /// \param vGlobalPosition The world position where to initiate the break
  /// \param fImpactRadius The radius of the impact area
  /// \param vImpulse The impulse to apply to the shards
  /// \param fMakeDynamicRadius Radius around the break point where shards become dynamic
  void ShatterRadial(const WVec3& vGlobalPosition, float fImpactRadius, const WVec3& vImpulse, float fMakeDynamicRadius); // [ scriptable ]

private:
  friend class WShatterTask;

  void PrepareBreakAsync(WDynamicArray<JPH::Ref<JPH::ConvexShape>>& out_Shapes, WArrayPtr<const WShatterPoint> points);
  void ApplyBreak(WArrayPtr<JPH::Ref<JPH::ConvexShape>> shapes, const WMeshResourceHandle& hMesh, const WVec3& vImpulse);
  void DebugDraw();
  void Cleanup();
  void ReinitMeshes();
  WMeshResourceHandle CreateShardsMesh() const;
  void PrepareShardColliders(WUInt32 uiFirstShard, WDynamicArray<JPH::Ref<JPH::ConvexShape>>& out_Shapes) const;
  void CreateShardColliders(WUInt32 uiFirstShard, WArrayPtr<JPH::Ref<JPH::ConvexShape>> shapes);
  void DestroyAllShardColliders();
  void DestroyShardCollider(WUInt32 uiShardIdx, JPH::BodyInterface& jphBodies, bool bUpdateVis);
  void RetrieveShardTransforms();
  void ApplyImpulse(const WVec3& vImpulse, WUInt32 uiFirstShard = 0);
  void UpdateShardColliders();
  void WakeUpBodies();
  bool IsPointOnSlab(const WVec3& vGlobalPosition) const;
  WUInt32 FindClosestShard(const WVec3& vGlobalPosition) const;

  void OnMsgPhysicsAddImpulse(WMsgPhysicsAddImpulse& ref_msg);                             // [ msg handler ]
  void OnMsgPhysicContactMsg(WMsgPhysicContact& ref_msg);                                  // [ msg handler ]
  void OnMsgPhysicCharacterContact(WMsgPhysicCharacterContact& ref_msg);                   // [ msg handler ]
  void OnMsgCustomInstanceDataOffsetChanged(WMsgCustomInstanceDataOffsetChanged& ref_msg); // [ msg handler ]

  void OnMsgExtractRenderData(WMsgExtractRenderData& msg) const;
  void BuildMeshResourceFromGeometry(WGeometry& Geometry, WMeshResourceDescriptor& MeshDesc, bool bWithSkinningData) const;
  const WJoltMaterial* GetPhysicsMaterial();

  float m_fGravityFactor = 1.0f;
  float m_fWidth = 1.0f;
  float m_fHeight = 1.0f;
  float m_fThickness = 0.02f;
  WVec2 m_vUvScale = WVec2(1.0f);
  float m_fContactReportForceThreshold = 0.0f;

  WBreakable2D m_Breakable;

  WMeshResourceHandle m_hMesh;
  WMaterialResourceHandle m_hMaterial;
  WSurfaceResourceHandle m_hSurface;

  bool m_bReinitMeshes = true;
  mutable WUInt8 m_uiShardsSleeping = 200;
  mutable WInstanceDataOffset m_InstanceDataOffset;

  static WAtomicInteger32 s_iShardMeshCounter;

  WUInt8 m_uiCollisionLayerStatic = 0;
  WUInt8 m_uiCollisionLayerDynamic = 0;

  WUInt32 m_uiUserDataIndexStatic = WInvalidIndex;
  WUInt32 m_uiUserDataIndexDynamic = WInvalidIndex;
  WUInt32 m_uiObjectFilterID = WInvalidIndex;

  WDynamicArray<WUInt32> m_ShardBodyIDs;

  WSkinningState m_SkinningState;
  WBoundingBoxSphere m_Bounds;

  WEnum<WJoltBreakableShape> m_Shape;
  WBitflags<WJoltBreakableSlabFlags> m_Flags;

  WHybridArray<WShatterPoint, 2> m_ShatterPoints;
  WSharedPtr<WShatterTask> m_pShatterTask;
};
