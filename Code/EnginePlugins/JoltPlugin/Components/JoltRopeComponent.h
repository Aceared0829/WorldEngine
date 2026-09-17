#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/ComponentManager.h>
#include <JoltPlugin/JoltPluginDLL.h>

struct WMsgPhysicsAddImpulse;
struct WJoltMsgDisconnectConstraints;
class WJoltMaterial;

namespace JPH
{
  class Constraint;
  class Ragdoll;
}

using WSurfaceResourceHandle = WTypedResourceHandle<class WSurfaceResource>;

/// How a rope end gets attached to the anchor point.
struct WJoltRopeAnchorConstraintMode
{
  using StorageType = WInt8;

  enum Enum
  {
    None,  ///< The rope is not attached at this anchor and will fall down here once simulation starts.
    Point, ///< The rope end can rotate freely around the anchor point.
    Fixed, ///< The rope end can neither move nor rotate at the anchor point.
    Cone,  ///< The rope end can rotate up to a maximum angle at the anchor point.

    Default = Point
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_JOLTPLUGIN_DLL, WJoltRopeAnchorConstraintMode);

//////////////////////////////////////////////////////////////////////////

class W_JOLTPLUGIN_DLL WJoltRopeComponentManager : public WComponentManager<class WJoltRopeComponent, WBlockStorageType::Compact>
{
public:
  WJoltRopeComponentManager(WWorld* pWorld);
  ~WJoltRopeComponentManager();

  virtual void Initialize() override;

private:
  void Update(const WWorldModule::UpdateContext& context);
};

//////////////////////////////////////////////////////////////////////////

/// Creates a physically simulated rope that is made up of multiple segments.
///
/// The rope will be created between two anchor points. The component requires at least one anchor to be provided,
/// if no second anchor is given, the rope's owner is used as the second.
///
/// If the anchors themselves are physically simulated bodies, the rope will attach to those bodies,
/// making it possible to constrain physics objects with a rope.
class W_JOLTPLUGIN_DLL WJoltRopeComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(WJoltRopeComponent, WComponent, WJoltRopeComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // WJoltRopeComponent

public:
  WJoltRopeComponent();
  ~WJoltRopeComponent();

  /// How strongly gravity pulls the rope down.
  void SetGravityFactor(float fGravity);                      // [ property ]
  float GetGravityFactor() const { return m_fGravityFactor; } // [ property ]

  /// The WSurfaceResource to be used on the rope physics bodies.
  void SetSurfaceFile(WStringView sFile); // [ property ]
  WStringView GetSurfaceFile() const;     // [ property ]

  /// Defines which other physics objects the rope collides with.
  WUInt8 m_uiCollisionLayer = 0; // [ property ]

  /// Of how many pieces the rope is made up.
  WUInt16 m_uiPieces = 16; // [ property ]

  /// How thick the simulated rope is. This is independent of the rope render thickness.
  float m_fThickness = 0.05f; // [ property ]

  /// How much the rope should sag. A value of 0 means it should be absolutely straight.
  float m_fSlack = 0.3f; // [ property ]

  /// If enabled, a more precise simulation method is used, preventing the rope from tunneling through walls.
  /// This comes at an extra performance cost.
  bool m_bCCD = false; // [ property ]

  /// How much each rope segment may bend.
  WAngle m_MaxBend = WAngle::MakeFromDegree(30); // [ property ]

  /// How much each rope segment may twist.
  WAngle m_MaxTwist = WAngle::MakeFromDegree(15); // [ property ]

  WUInt8 m_uiWeightCategory = 0;                   // [ property ]
  WFloat16 m_fWeightScale = 1.0f;                  // [ property ]
  WFloat16 m_fWeightMass = 5.0f;                   // [ property ]

  /// Sets the anchor 1 references by object GUID.
  void SetAnchor1Reference(const char* szReference); // [ property ]

  /// Sets the anchor 2 references by object GUID.
  void SetAnchor2Reference(const char* szReference); // [ property ]

  /// Sets the anchor 1 reference.
  void SetAnchor1(WGameObjectHandle hActor);

  /// Sets the anchor 2 reference.
  void SetAnchor2(WGameObjectHandle hActor);

  /// Adds an impulse (like an impact) to the rope.
  void AddImpulseAtPos(WMsgPhysicsAddImpulse& ref_msg);

  /// Configures how the rope is attached at anchor 1.
  void SetAnchor1ConstraintMode(WEnum<WJoltRopeAnchorConstraintMode> mode);                                 // [ property ]
  WEnum<WJoltRopeAnchorConstraintMode> GetAnchor1ConstraintMode() const { return m_Anchor1ConstraintMode; } // [ property ]

  /// Configures how the rope is attached at anchor 2.
  void SetAnchor2ConstraintMode(WEnum<WJoltRopeAnchorConstraintMode> mode);                                 // [ property ]
  WEnum<WJoltRopeAnchorConstraintMode> GetAnchor2ConstraintMode() const { return m_Anchor2ConstraintMode; } // [ property ]

  /// Makes sure that the rope's connection to a removed body also gets removed.
  void OnJoltMsgDisconnectConstraints(WJoltMsgDisconnectConstraints& ref_msg); // [ msg handler ]

private:
  void CreateRope();
  WResult CreateSegmentTransforms(WDynamicArray<WTransform>& transforms, float& out_fPieceLength, WGameObjectHandle hAnchor1, WGameObjectHandle hAnchor2);
  void DestroyPhysicsShapes();
  void Update();
  void SendPreviewPose();
  const WJoltMaterial* GetJoltMaterial();
  JPH::Constraint* CreateConstraint(const WGameObjectHandle& hTarget, const WTransform& dstLoc, WUInt32 uiBodyID, WJoltRopeAnchorConstraintMode::Enum mode, WUInt32& out_uiConnectedToBodyID);
  void UpdatePreview();

  float GetWeight_Scale() const { return m_fWeightScale; }
  float GetWeight_Mass() const { return m_fWeightMass; }
  void SetWeight_Scale(float fValue) { m_fWeightScale = fValue; }
  void SetWeight_Mass(float fValue) { m_fWeightMass = fValue; }

  WSurfaceResourceHandle m_hSurface;

  WGameObjectHandle m_hAnchor1;
  WGameObjectHandle m_hAnchor2;

  WEnum<WJoltRopeAnchorConstraintMode> m_Anchor1ConstraintMode; // [ property ]
  WEnum<WJoltRopeAnchorConstraintMode> m_Anchor2ConstraintMode; // [ property ]

  float m_fMaxForcePerFrame = 0.0f;
  float m_fBendStiffness = 0.0f;
  WUInt32 m_uiObjectFilterID = WInvalidIndex;
  WUInt32 m_uiUserDataIndex = WInvalidIndex;
  bool m_bSelfCollision = false;
  float m_fGravityFactor = 1.0f;
  WUInt32 m_uiPreviewHash = 0;

  JPH::Ragdoll* m_pRagdoll = nullptr;
  JPH::Constraint* m_pConstraintAnchor1 = nullptr;
  JPH::Constraint* m_pConstraintAnchor2 = nullptr;
  WUInt32 m_uiAnchor1BodyID = WInvalidIndex;
  WUInt32 m_uiAnchor2BodyID = WInvalidIndex;

private:
  const char* DummyGetter() const { return nullptr; }
};
