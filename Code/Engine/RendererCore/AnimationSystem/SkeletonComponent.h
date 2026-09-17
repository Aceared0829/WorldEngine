#pragma once

#include <Foundation/Math/Declarations.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>

struct WMsgQueryAnimationSkeleton;

using WVisualizeSkeletonComponentManager = WComponentManagerSimple<class WSkeletonComponent, WComponentUpdateType::Always, WBlockStorageType::Compact>;

/// Uses debug rendering to visualize various aspects of an animation skeleton.
///
/// This is meant for visually inspecting skeletons. It is used by the main skeleton editor,
/// but can also be added to a scene or added to an animated mesh on-demand.
///
/// There are different options what to visualize and also to highlight certain bones.
class W_RENDERERCORE_DLL WSkeletonComponent : public WRenderComponent
{
  W_DECLARE_COMPONENT_TYPE(WSkeletonComponent, WRenderComponent, WVisualizeSkeletonComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;


  //////////////////////////////////////////////////////////////////////////
  // WRenderComponent

public:
  virtual WResult GetLocalBounds(WBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, WMsgUpdateLocalBounds& ref_msg) override;

  //////////////////////////////////////////////////////////////////////////
  // WSkeletonComponent

public:
  WSkeletonComponent();
  ~WSkeletonComponent();

  void SetSkeleton(const WSkeletonResourceHandle& hResource);                // [ property ]
  const WSkeletonResourceHandle& GetSkeleton() const { return m_hSkeleton; } // [ property ]

  /// Sets a semicolon-separated list of bone names that should be highlighted.
  ///
  /// Set it to "*" to highlight all bones.
  /// Set it to empty to not highlight any bone.
  /// Set it to "BoneA;BoneB" to highlight the bones with name "BoneA" and "BoneB".
  void SetBonesToHighlight(const char* szFilter); // [ property ]
  const char* GetBonesToHighlight() const;        // [ property ]

  bool m_bVisualizeBones = true;                  // [ property ]
  bool m_bVisualizeColliders = false;             // [ property ]
  bool m_bVisualizeJoints = false;                // [ property ]
  bool m_bVisualizeSwingLimits = false;           // [ property ]
  bool m_bVisualizeTwistLimits = false;           // [ property ]

protected:
  void Update();
  void VisualizeSkeletonDefaultState();
  void OnAnimationPoseUpdated(WMsgAnimationPoseUpdated& msg); // [ msg handler ]

  void BuildSkeletonVisualization(WMsgAnimationPoseUpdated& msg);
  void BuildColliderVisualization(WMsgAnimationPoseUpdated& msg);
  void BuildJointVisualization(WMsgAnimationPoseUpdated& msg);

  void OnQueryAnimationSkeleton(WMsgQueryAnimationSkeleton& msg);
  WDebugRendererLine& AddLine(const WVec3& vStart, const WVec3& vEnd, const WColor& color);

  WSkeletonResourceHandle m_hSkeleton;
  WTransform m_RootTransform = WTransform::MakeIdentity();
  WUInt32 m_uiSkeletonChangeCounter = 0;
  WString m_sBonesToHighlight;

  WBoundingBox m_MaxBounds;
  WDynamicArray<WDebugRendererLine> m_LinesSkeleton;

  struct SphereShape
  {
    WTransform m_Transform;
    WBoundingSphere m_Shape;
    WColor m_Color;
  };

  struct BoxShape
  {
    WTransform m_Transform;
    WBoundingBox m_Shape;
    WColor m_Color;
  };

  struct CapsuleShape
  {
    WTransform m_Transform;
    float m_fLength;
    float m_fRadius;
    WColor m_Color;
  };

  struct AngleShape
  {
    WTransform m_Transform;
    WColor m_Color;
    WAngle m_StartAngle;
    WAngle m_EndAngle;
  };

  struct ConeLimitShape
  {
    WTransform m_Transform;
    WColor m_Color;
    WAngle m_Angle1;
    WAngle m_Angle2;
  };

  struct CylinderShape
  {
    WTransform m_Transform;
    WColor m_Color;
    float m_fRadius1;
    float m_fRadius2;
    float m_fLength;
  };

  WDynamicArray<SphereShape> m_SpheresShapes;
  WDynamicArray<BoxShape> m_BoxShapes;
  WDynamicArray<CapsuleShape> m_CapsuleShapes;
  WDynamicArray<AngleShape> m_AngleShapes;
  WDynamicArray<ConeLimitShape> m_ConeLimitShapes;
  WDynamicArray<CylinderShape> m_CylinderShapes;
};
