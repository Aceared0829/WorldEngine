#include <TerrainPlugin/TerrainPluginPCH.h>

#include <Core/Messages/TransformChangedMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Reflection/Implementation/PropertyAttributes.h>
#include <Foundation/Types/TagRegistry.h>
#include <RendererCore/Components/SplineComponent.h>
#include <TerrainPlugin/Components/TerrainBrushBaseComponent.h>
#include <TerrainPlugin/TerrainSystem.h>

// clang-format off
W_BEGIN_ABSTRACT_COMPONENT_TYPE(WTerrainBrushBaseComponent, 2)
{
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgTransformChanged, OnMsgTransformChanged),
    W_MESSAGE_HANDLER(WMsgSplineChanged, OnMsgSplineChanged),
  }
  W_END_MESSAGEHANDLERS;
}
W_END_ABSTRACT_COMPONENT_TYPE;
// clang-format on

WTerrainBrushBaseComponent::WTerrainBrushBaseComponent() = default;
WTerrainBrushBaseComponent::~WTerrainBrushBaseComponent() = default;

void WTerrainBrushBaseComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();
  s << m_fHalfSizeX;
  s << m_fInnerRadius;
  s << m_fOuterRadius;
  s << m_fFalloff;
  s << m_fNoiseStrength;
  s << m_fNoiseFrequency;
  s << m_uiMaterialIndex;
  s << m_fMaterialStrength;
  s << m_iPriority;
  m_Tags.Save(s);

  s << m_bAffectPatches;
  s << m_bAffectVolumes;
}

void WTerrainBrushBaseComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  s >> m_fHalfSizeX;
  s >> m_fInnerRadius;
  s >> m_fOuterRadius;
  s >> m_fFalloff;
  s >> m_fNoiseStrength;
  s >> m_fNoiseFrequency;
  s >> m_uiMaterialIndex;
  s >> m_fMaterialStrength;
  s >> m_iPriority;
  m_Tags.Load(s, WTagRegistry::GetGlobalRegistry());

  if (uiVersion >= 2)
  {
    s >> m_bAffectPatches;
    s >> m_bAffectVolumes;
  }
}

void WTerrainBrushBaseComponent::OnActivated()
{
  SUPER::OnActivated();
  GetOwner()->EnableStaticTransformChangesNotifications();
  RefreshBrushes();
}

void WTerrainBrushBaseComponent::OnDeactivated()
{
  ClearBrushes();
  SUPER::OnDeactivated();
}

void WTerrainBrushBaseComponent::OnMsgTransformChanged(WMsgTransformChanged& msg)
{
  RefreshBrushes();
}

void WTerrainBrushBaseComponent::OnMsgSplineChanged(WMsgSplineChanged& msg)
{
  RefreshBrushes();
}

void WTerrainBrushBaseComponent::ClearBrushes()
{
  if (m_BrushIndices.IsEmpty())
    return;

  auto* pSystem = GetWorld()->GetOrCreateModule<WTerrainSystem>();
  for (WUInt32 uiIdx : m_BrushIndices)
    pSystem->RemoveBrushData(uiIdx);
  m_BrushIndices.Clear();
}

void WTerrainBrushBaseComponent::FillBrush(WTerrainData_Brush& brush, const WTransform& transform, float fHalfSizeX)
{
  brush.m_vPosition = transform.m_vPosition;
  brush.m_qRotation = transform.m_qRotation;
  brush.m_vHalfExtents.x = fHalfSizeX;
  brush.m_fInnerRadius = m_fInnerRadius;
  brush.m_fOuterRadius = m_fOuterRadius;
  brush.m_fFalloff = m_fFalloff;
  brush.m_uiMaterialIndex = m_uiMaterialIndex;
  brush.m_fMaterialStrength = m_fMaterialStrength;
  brush.m_bAffectHeightfields = m_bAffectPatches;
  brush.m_bAffectVoxels = m_bAffectVolumes;
  brush.m_fNoiseStrength = m_fNoiseStrength;
  brush.m_fNoiseFrequency = m_fNoiseFrequency;
  brush.m_iPriority = m_iPriority;
  brush.m_Tags = m_Tags;
  FillBrushSpecificProperties(brush, fHalfSizeX);
}

void WTerrainBrushBaseComponent::RefreshBrushes()
{
  ClearBrushes();

  if (!IsActiveAndInitialized())
    return;

  auto* pSystem = GetWorld()->GetOrCreateModule<WTerrainSystem>();

  const WSplineComponent* pSpline = nullptr;
  if (GetOwner()->TryGetComponentOfBaseType(pSpline))
  {
    const float fTotalLength = pSpline->GetTotalLength();
    if (fTotalLength <= 0.0f)
      return;

    constexpr float fMaxStep = 10.0f;
    constexpr float fMinStep = 0.5f;
    constexpr float fTolerance = 0.1f;

    auto PlaceBrush = [&](float d0, float d1)
    {
      const float dMid = (d0 + d1) * 0.5f;
      const float fSegmentHalfLength = (d1 - d0) * 0.5f;
      const WTransform trans = pSpline->GetTransformAtDistance(dMid, WSplineComponentSpace::Global);

      const WUInt32 uiIdx = pSystem->CreateBrushData();
      m_BrushIndices.PushBack(uiIdx);
      FillBrush(pSystem->ModifyBrushData(uiIdx), trans, fSegmentHalfLength);
    };

    auto Subdivide = [&](auto& self, float d0, float d1) -> void
    {
      const float fLength = d1 - d0;
      const float dMid = (d0 + d1) * 0.5f;

      if (fLength <= fMinStep)
      {
        PlaceBrush(d0, d1);
        return;
      }

      if (fLength <= fMaxStep)
      {
        const WVec3 p0 = pSpline->GetTransformAtDistance(d0, WSplineComponentSpace::Global).m_vPosition;
        const WVec3 p1 = pSpline->GetTransformAtDistance(d1, WSplineComponentSpace::Global).m_vPosition;
        const WVec3 pMidActual = pSpline->GetTransformAtDistance(dMid, WSplineComponentSpace::Global).m_vPosition;

        if ((pMidActual - (p0 + p1) * 0.5f).GetLength() <= fTolerance)
        {
          PlaceBrush(d0, d1);
          return;
        }
      }

      self(self, d0, dMid);
      self(self, dMid, d1);
    };

    Subdivide(Subdivide, 0.0f, fTotalLength);
  }
  else
  {
    const WUInt32 uiIdx = pSystem->CreateBrushData();
    m_BrushIndices.PushBack(uiIdx);
    FillBrush(pSystem->ModifyBrushData(uiIdx), GetOwner()->GetGlobalTransform(), m_fHalfSizeX);
  }
}

void WTerrainBrushBaseComponent::SetHalfSizeX(float fSize)
{
  if (m_fHalfSizeX == fSize)
    return;
  m_fHalfSizeX = fSize;
  RefreshBrushes();
}

void WTerrainBrushBaseComponent::SetInnerRadius(float fRadius)
{
  if (m_fInnerRadius == fRadius)
    return;
  m_fInnerRadius = fRadius;
  RefreshBrushes();
}

void WTerrainBrushBaseComponent::SetOuterRadius(float fRadius)
{
  if (m_fOuterRadius == fRadius)
    return;
  m_fOuterRadius = fRadius;
  RefreshBrushes();
}

void WTerrainBrushBaseComponent::SetFalloff(float fFalloff)
{
  if (m_fFalloff == fFalloff)
    return;
  m_fFalloff = fFalloff;
  RefreshBrushes();
}

void WTerrainBrushBaseComponent::SetMaterialIndex(WUInt8 uiIndex)
{
  if (m_uiMaterialIndex == uiIndex)
    return;
  m_uiMaterialIndex = uiIndex;
  RefreshBrushes();
}

void WTerrainBrushBaseComponent::SetMaterialStrength(float fStrength)
{
  if (m_fMaterialStrength == fStrength)
    return;
  m_fMaterialStrength = fStrength;
  RefreshBrushes();
}

void WTerrainBrushBaseComponent::SetAffectPatches(bool b)
{
  if (m_bAffectPatches == b)
    return;
  m_bAffectPatches = b;
  RefreshBrushes();
}

void WTerrainBrushBaseComponent::SetAffectVolumes(bool b)
{
  if (m_bAffectVolumes == b)
    return;
  m_bAffectVolumes = b;
  RefreshBrushes();
}

void WTerrainBrushBaseComponent::SetNoiseStrength(float fNoise)
{
  if (m_fNoiseStrength == fNoise)
    return;
  m_fNoiseStrength = fNoise;
  RefreshBrushes();
}

void WTerrainBrushBaseComponent::SetNoiseFrequency(float fNoise)
{
  if (m_fNoiseFrequency == fNoise)
    return;
  m_fNoiseFrequency = fNoise;
  RefreshBrushes();
}

void WTerrainBrushBaseComponent::SetPriority(WInt8 iPriority)
{
  if (m_iPriority == iPriority)
    return;
  m_iPriority = iPriority;
  RefreshBrushes();
}

void WTerrainBrushBaseComponent::Reflection_SetTag(const char* szTagName)
{
  if (WStringUtils::IsNullOrEmpty(szTagName))
    return;
  const WTag& tag = WTagRegistry::GetGlobalRegistry().RegisterTag(szTagName);
  if (m_Tags.IsSet(tag))
    return;
  m_Tags.Set(tag);
  RefreshBrushes();
}

void WTerrainBrushBaseComponent::Reflection_RemoveTag(const char* szTagName)
{
  if (WStringUtils::IsNullOrEmpty(szTagName))
    return;
  if (const WTag* pTag = WTagRegistry::GetGlobalRegistry().GetTagByName(WTempHashedString(szTagName)))
  {
    if (!m_Tags.IsSet(*pTag))
      return;
    m_Tags.Remove(*pTag);
    RefreshBrushes();
  }
}
