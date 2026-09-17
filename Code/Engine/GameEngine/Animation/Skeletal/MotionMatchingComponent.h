// #pragma once
//
// #include <GameEngine/GameEngineDLL.h>
// #include <RendererCore/AnimationSystem/AnimationGraph/AnimationClipSampler.h>
// #include <RendererCore/AnimationSystem/AnimationPose.h>
// #include <RendererCore/Meshes/SkinnedMeshComponent.h>
//
// using WAnimationClipResourceHandle = WTypedResourceHandle<class WAnimationClipResource>;
// using WSkeletonResourceHandle = WTypedResourceHandle<class WSkeletonResource>;
//
// using WMotionMatchingComponentManager = WComponentManagerSimple<class WMotionMatchingComponent, WComponentUpdateType::WhenSimulating> ;
//
// class W_GAMEENGINE_DLL WMotionMatchingComponent : public WSkinnedMeshComponent
//{
//   W_DECLARE_COMPONENT_TYPE(WMotionMatchingComponent, WSkinnedMeshComponent, WMotionMatchingComponentManager);
//
//   //////////////////////////////////////////////////////////////////////////
//   // WComponent
//
// public:
//   virtual void SerializeComponent(WWorldWriter& stream) const override;
//   virtual void DeserializeComponent(WWorldReader& stream) override;
//
// protected:
//   virtual void OnSimulationStarted() override;
//
//
//   //////////////////////////////////////////////////////////////////////////
//   // WMotionMatchingComponent
//
// public:
//   WMotionMatchingComponent();
//   ~WMotionMatchingComponent();
//
//   void SetAnimation(WUInt32 uiIndex, const WAnimationClipResourceHandle& hResource);
//   WAnimationClipResourceHandle GetAnimation(WUInt32 uiIndex) const;
//
// protected:
//   void Update();
//
//   WUInt32 Animations_GetCount() const;                          // [ property ]
//   const char* Animations_GetValue(WUInt32 uiIndex) const;       // [ property ]
//   void Animations_SetValue(WUInt32 uiIndex, const char* value); // [ property ]
//   void Animations_Insert(WUInt32 uiIndex, const char* value);   // [ property ]
//   void Animations_Remove(WUInt32 uiIndex);                      // [ property ]
//
//   void ConfigureInput();
//   WVec3 GetInputDirection() const;
//   WQuat GetInputRotation() const;
//
//   WAnimationPose m_AnimationPose;
//   WSkeletonResourceHandle m_hSkeleton;
//
//   WDynamicArray<WAnimationClipResourceHandle> m_Animations;
//
//   WVec3 m_vLeftFootPos;
//   WVec3 m_vRightFootPos;
//
//   struct MotionData
//   {
//     WUInt16 m_uiAnimClipIndex;
//     WUInt16 m_uiKeyframeIndex;
//     WVec3 m_vLeftFootPosition;
//     WVec3 m_vLeftFootVelocity;
//     WVec3 m_vRightFootPosition;
//     WVec3 m_vRightFootVelocity;
//     WVec3 m_vRootVelocity;
//   };
//
//   struct TargetKeyframe
//   {
//     WUInt16 m_uiAnimClip;
//     WUInt16 m_uiKeyframe;
//   };
//
//   TargetKeyframe m_Keyframe0;
//   TargetKeyframe m_Keyframe1;
//   float m_fKeyframeLerp = 0.0f;
//
//   TargetKeyframe FindNextKeyframe(const TargetKeyframe& current, const WVec3& vTargetDir) const;
//
//   WDynamicArray<MotionData> m_MotionData;
//
//   static void PrecomputeMotion(WDynamicArray<MotionData>& motionData, WTempHashedString jointName1, WTempHashedString jointName2,
//     const WAnimationClipResourceDescriptor& animClip, WUInt16 uiAnimClipIndex, const WSkeleton& skeleton);
//
//   WUInt32 FindBestKeyframe(const TargetKeyframe& current, WVec3 vLeftFootPosition, WVec3 vRightFootPosition, WVec3 vTargetDir) const;
// };
