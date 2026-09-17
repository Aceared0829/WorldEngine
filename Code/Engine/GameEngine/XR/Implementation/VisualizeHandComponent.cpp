#include <GameEngine/GameEnginePCH.h>

#include <Core/Graphics/Geometry.h>
#include <Foundation/Configuration/Singleton.h>
#include <GameEngine/XR/VisualizeHandComponent.h>
#include <GameEngine/XR/XRHandTrackingInterface.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Debug/DebugRendererContext.h>

// clang-format off
W_BEGIN_COMPONENT_TYPE(WVisualizeHandComponent, 1, WComponentMode::Static)
{
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("XR"),
    new WInDevelopmentAttribute(WInDevelopmentAttribute::Phase::Beta),
  }
  W_END_ATTRIBUTES;
}
W_END_COMPONENT_TYPE;
// clang-format on

WVisualizeHandComponent::WVisualizeHandComponent() = default;
WVisualizeHandComponent::~WVisualizeHandComponent() = default;

void WVisualizeHandComponent::Update()
{
  WXRHandTrackingInterface* pXRHand = WSingletonRegistry::GetSingletonInstance<WXRHandTrackingInterface>();

  if (!pXRHand)
    return;

  WTempHybridArray<WXRHandBone, 6> bones;
  for (WXRHand::Enum hand : {WXRHand::Left, WXRHand::Right})
  {
    for (WUInt32 uiPart = 0; uiPart < WXRHandPart::COUNT; ++uiPart)
    {
      WXRHandPart::Enum part = static_cast<WXRHandPart::Enum>(uiPart);
      if (pXRHand->TryGetBoneTransforms(hand, part, WXRTransformSpace::Global, bones) == WXRHandTrackingInterface::HandPartTrackingState::Tracked)
      {
        WTempHybridArray<WDebugRendererLine, 6> m_Lines;
        for (WUInt32 uiBone = 0; uiBone < bones.GetCount(); uiBone++)
        {
          const WXRHandBone& bone = bones[uiBone];
          WBoundingSphere sphere = WBoundingSphere::MakeFromCenterAndRadius(WVec3::MakeZero(), bone.m_fRadius);
          WDebugRenderer::DrawLineSphere(GetWorld(), sphere, WColor::Aquamarine, bone.m_Transform);

          if (uiBone + 1 < bones.GetCount())
          {
            const WXRHandBone& nextBone = bones[uiBone + 1];
            m_Lines.PushBack(WDebugRendererLine(bone.m_Transform.m_vPosition, nextBone.m_Transform.m_vPosition));
          }
        }
        WDebugRenderer::DrawLines(GetWorld(), m_Lines, WColor::IndianRed);
      }
    }
  }
}

W_STATICLINK_FILE(GameEngine, GameEngine_XR_Implementation_VisualizeHandComponent);
