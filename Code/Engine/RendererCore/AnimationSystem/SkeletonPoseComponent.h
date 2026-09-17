#pragma once

#include <Core/World/ComponentManager.h>
#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Types/RangeView.h>
#include <RendererCore/AnimationSystem/EditableSkeleton.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

class WSkeletonPoseComponentManager : public WComponentManager<class WSkeletonPoseComponent, WBlockStorageType::Compact>
{
public:
  using SUPER = WComponentManager<WSkeletonPoseComponent, WBlockStorageType::Compact>;

  WSkeletonPoseComponentManager(WWorld* pWorld)
    : SUPER(pWorld)
  {
  }

  void Update(const WWorldModule::UpdateContext& context);
  void EnqueueUpdate(WComponentHandle hComponent);

private:
  mutable WMutex m_Mutex;
  WDeque<WComponentHandle> m_RequireUpdate;

protected:
  virtual void Initialize() override;
};

//////////////////////////////////////////////////////////////////////////

/// Which pose to apply to an animated mesh.
struct WSkeletonPoseMode
{
  using StorageType = WUInt8;

  enum Enum
  {
    CustomPose, ///< Set a custom pose on the mesh.
    RestPose,   ///< Set the rest pose (bind pose) on the mesh.
    Disabled,   ///< Don't set any pose.
    Default = CustomPose
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_RENDERERCORE_DLL, WSkeletonPoseMode);

/// Used in conjunction with an WAnimatedMeshComponent to set a specific pose for the animated mesh.
///
/// This component is used to set one, static pose for an animated mesh. The pose is applied once at startup.
/// This can be used to either just pose a mesh in a certain way, or to set a start pose that is then used
/// by other systems, for example a ragdoll component, to generate further poses.
///
/// The component needs to be attached to the same game object where the animated mesh component is attached.
class W_RENDERERCORE_DLL WSkeletonPoseComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(WSkeletonPoseComponent, WComponent, WSkeletonPoseComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // WSkeletonPoseComponent

public:
  WSkeletonPoseComponent();
  ~WSkeletonPoseComponent();

  /// Sets the WSkeletonResource to use.
  void SetSkeleton(const WSkeletonResourceHandle& hResource);                // [ property ]
  const WSkeletonResourceHandle& GetSkeleton() const { return m_hSkeleton; } // [ property ]

  /// Configures which pose to apply to the animated mesh.
  void SetPoseMode(WEnum<WSkeletonPoseMode> mode);
  WEnum<WSkeletonPoseMode> GetPoseMode() const { return m_PoseMode; }

  const WRangeView<const char*, WUInt32> GetBones() const;   // [ property ] (exposed bones)
  void SetBone(const char* szKey, const WVariant& value);     // [ property ] (exposed bones)
  void RemoveBone(const char* szKey);                          // [ property ] (exposed bones)
  bool GetBone(const char* szKey, WVariant& out_value) const; // [ property ] (exposed bones)

  /// Instructs the component to apply the pose to the animated mesh again.
  void ResendPose();

protected:
  void Update();
  void SendRestPose();
  void SendCustomPose();

  float m_fDummy = 0;
  WUInt8 m_uiResendPose = 0;
  WSkeletonResourceHandle m_hSkeleton;
  WArrayMap<WHashedString, WExposedBone> m_Bones; // [ property ]
  WEnum<WSkeletonPoseMode> m_PoseMode;             // [ property ]
};
