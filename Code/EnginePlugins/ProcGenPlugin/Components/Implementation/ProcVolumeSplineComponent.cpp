#include <ProcGenPlugin/ProcGenPluginPCH.h>

#include <Core/Messages/TransformChangedMessage.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <ProcGenPlugin/Components/ProcVolumeSplineComponent.h>
#include <ProcGenPlugin/Components/VolumeCollection.h>
#include <RendererCore/Components/SplineComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/RenderData.h>

// clang-format off
W_BEGIN_COMPONENT_TYPE(WProcVolumeSplineComponent, 1, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new WDefaultValueAttribute(5.0f), new WClampValueAttribute(0.0f, WVariant())),
    W_ACCESSOR_PROPERTY("Falloff", GetFalloff, SetFalloff)->AddAttributes(new WDefaultValueAttribute(0.5f), new WClampValueAttribute(0.0f, 1.0f)),
  }
  W_END_PROPERTIES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgSplineChanged, OnMsgSplineChanged),
    W_MESSAGE_HANDLER(WMsgUpdateLocalBounds, OnMsgUpdateLocalBounds),
    W_MESSAGE_HANDLER(WMsgExtractVolumes, OnMsgExtractVolumes),
    W_MESSAGE_HANDLER(WMsgExtractRenderData, OnMsgExtractRenderData),
  }
  W_END_MESSAGEHANDLERS;
}
W_END_COMPONENT_TYPE
// clang-format on

WProcVolumeSplineComponent::WProcVolumeSplineComponent() = default;
WProcVolumeSplineComponent::~WProcVolumeSplineComponent() = default;

void WProcVolumeSplineComponent::SetRadius(float fRadius)
{
  if (m_fRadius != fRadius)
  {
    m_fRadius = fRadius;

    if (IsActiveAndInitialized())
    {
      GetOwner()->UpdateLocalBounds();
    }

    InvalidateArea();

    m_DebugLines.Clear();
  }
}

void WProcVolumeSplineComponent::SetFalloff(float fFalloff)
{
  if (m_fFalloff != fFalloff)
  {
    m_fFalloff = fFalloff;

    InvalidateArea();
  }
}

void WProcVolumeSplineComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  WStreamWriter& s = inout_stream.GetStream();

  s << m_fRadius;
  s << m_fFalloff;
}

void WProcVolumeSplineComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  WStreamReader& s = inout_stream.GetStream();

  s >> m_fRadius;
  s >> m_fFalloff;
}

void WProcVolumeSplineComponent::OnMsgSplineChanged(WMsgSplineChanged& ref_msg)
{
  // An invalid change counter indicates that this msg came from a spline node before the actual spline has been updated. Ignore that here.
  if (ref_msg.m_uiChangeCounter == WInvalidIndex || ref_msg.m_uiChangeCounter == m_uiLastChangeCounter)
    return;

  m_uiLastChangeCounter = ref_msg.m_uiChangeCounter;

  GetOwner()->UpdateLocalBounds();

  InvalidateArea();

  m_DebugLines.Clear();
}

void WProcVolumeSplineComponent::OnMsgUpdateLocalBounds(WMsgUpdateLocalBounds& ref_msg) const
{
  const WSplineComponent* pSplineComponent = GetSplineComponent();
  if (pSplineComponent == nullptr)
    return;

  WSimdBBoxSphere bounds;
  if (pSplineComponent->GetSpline().CalculateBounds(bounds).Failed())
    return;

  bounds.m_BoxHalfExtents += WSimdVec4f(m_fRadius);
  bounds.m_CenterAndRadius.SetW(bounds.m_CenterAndRadius.w() + WSimdFloat(m_fRadius));

  ref_msg.AddBounds(WSimdConversion::ToBBoxSphere(bounds), s_SpatialCategory);
}

void WProcVolumeSplineComponent::OnMsgExtractVolumes(WMsgExtractVolumes& ref_msg) const
{
  const WSplineComponent* pSplineComponent = GetSplineComponent();
  if (pSplineComponent == nullptr)
    return;

  if (pSplineComponent->GetSpline().m_ControlPoints.GetCount() < 2)
    return;

  ref_msg.m_pCollection->AddSpline(GetOwner()->GetGlobalTransformSimd(), pSplineComponent->GetSpline(), m_fRadius, m_BlendMode, m_fSortOrder, m_fValue, m_fFalloff);
}

void WProcVolumeSplineComponent::OnMsgExtractRenderData(WMsgExtractRenderData& ref_msg) const
{
  if (ref_msg.m_OverrideCategory != WDefaultRenderDataCategories::Selection)
    return;

  const WSplineComponent* pSplineComponent = GetSplineComponent();
  if (pSplineComponent == nullptr)
    return;

  auto& spline = pSplineComponent->GetSpline();
  if (spline.GetNumControlPoints() < 2)
    return;

  if (m_DebugLines.IsEmpty())
  {
    constexpr WUInt32 uiSteps = 8;
    constexpr WUInt32 uiRingSteps = 16;
    constexpr WAngle ringStepAngle = WAngle::MakeFromDegree(360.0f / (float)uiRingSteps);
    const WUInt32 uiNumSegments = spline.GetNumSegments();

    const WUInt32 uiNumRings = uiNumSegments * 2 + (spline.m_bClosed ? 0 : 3);
    const WUInt32 uiNumLines = uiNumSegments * uiSteps * 4 + uiNumRings * uiRingSteps;
    m_DebugLines.Reserve(uiNumLines);

    auto AddRing = [&](const WSimdTransform& t, WBasisAxis::Enum rightAxis, WBasisAxis::Enum upAxis, WUInt32 uiNumSteps)
    {
      const WSimdVec4f vRight = WSimdConversion::ToVec3(WBasisAxis::GetBasisVector(rightAxis)) * m_fRadius;
      const WSimdVec4f vUp = WSimdConversion::ToVec3(WBasisAxis::GetBasisVector(upAxis)) * m_fRadius;

      for (WUInt32 s = 0; s < uiNumSteps; ++s)
      {
        const float fS1 = static_cast<float>(s);
        const float fS2 = static_cast<float>(s + 1);

        const float fCos1 = WMath::Cos(fS1 * ringStepAngle);
        const float fCos2 = WMath::Cos(fS2 * ringStepAngle);

        const float fSin1 = WMath::Sin(fS1 * ringStepAngle);
        const float fSin2 = WMath::Sin(fS2 * ringStepAngle);

        auto& line = m_DebugLines.ExpandAndGetRef();
        line.m_start = WSimdConversion::ToVec3(t.TransformPosition(vRight * fSin1 + vUp * fCos1));
        line.m_end = WSimdConversion::ToVec3(t.TransformPosition(vRight * fSin2 + vUp * fCos2));
      }
    };

    const WSimdVec4f vOffsets[] = {
      WSimdVec4f(0, m_fRadius, 0),
      WSimdVec4f(0, -m_fRadius, 0),
      WSimdVec4f(0, 0, m_fRadius),
      WSimdVec4f(0, 0, -m_fRadius),
    };

    const float fStep = 1.0f / static_cast<float>(uiSteps);
    for (WUInt32 i = 0; i < uiNumSegments; ++i)
    {
      WSimdTransform t0 = spline.EvaluateTransform(static_cast<float>(i));

      AddRing(t0, WBasisAxis::PositiveY, WBasisAxis::PositiveZ, uiRingSteps);
      if (i == 0 && !spline.m_bClosed)
      {
        AddRing(t0, WBasisAxis::NegativeX, WBasisAxis::PositiveY, uiRingSteps / 2);
        AddRing(t0, WBasisAxis::NegativeX, WBasisAxis::PositiveZ, uiRingSteps / 2);
      }

      for (WUInt32 uiStep = 1; uiStep <= uiSteps; ++uiStep)
      {
        const float fT = fStep * static_cast<float>(uiStep);
        const WSimdTransform t1 = spline.EvaluateTransform(fT + i);
        for (const auto& offset : vOffsets)
        {
          auto& line = m_DebugLines.ExpandAndGetRef();
          line.m_start = WSimdConversion::ToVec3(t0.TransformPosition(offset));
          line.m_end = WSimdConversion::ToVec3(t1.TransformPosition(offset));
        }

        if (uiStep == uiSteps / 2)
        {
          AddRing(t1, WBasisAxis::PositiveY, WBasisAxis::PositiveZ, uiRingSteps);
        }

        t0 = t1;
      }

      if (i == uiNumSegments - 1 && !spline.m_bClosed)
      {
        AddRing(t0, WBasisAxis::PositiveX, WBasisAxis::PositiveY, uiRingSteps / 2);
        AddRing(t0, WBasisAxis::PositiveX, WBasisAxis::PositiveZ, uiRingSteps / 2);
        AddRing(t0, WBasisAxis::PositiveY, WBasisAxis::PositiveZ, uiRingSteps);
      }
    }

    W_ASSERT_DEBUG(m_DebugLines.GetCount() == uiNumLines, "Implementation error");
  }

  WColor c = WColorScheme::GetCategoryColor("Construction", WColorScheme::CategoryColorUsage::ViewportIcon);
  WDebugRenderer::DrawLines(GetWorld(), m_DebugLines, c, GetOwner()->GetGlobalTransform());
}

const WSplineComponent* WProcVolumeSplineComponent::GetSplineComponent() const
{
  const WGameObject* pObject = GetOwner();
  while (pObject != nullptr)
  {
    const WSplineComponent* pSplineComponent = nullptr;
    if (pObject->TryGetComponentOfBaseType(pSplineComponent))
    {
      return pSplineComponent;
    }
    pObject = pObject->GetParent();
  }

  return nullptr;
}


W_STATICLINK_FILE(ProcGenPlugin, ProcGenPlugin_Components_Implementation_ProcVolumeSplineComponent);
