#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <Core/Messages/TriggerMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Math/BoundingSphere.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <RendererCore/Components/LodComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/View.h>

W_RENDERERCORE_DLL WCVarFloat cvar_RenderingLodCoverageScale("Rendering.Lod.CoverageScale", 1.0f, WCVarFlags::Save, "Scales the screen space coverage that LOD components compute. Values below 1 switch to lower detail LODs earlier, values above 1 keep higher detail LODs longer.");
W_RENDERERCORE_DLL WCVarInt cvar_RenderingLodForce("Rendering.Lod.Force", -1, WCVarFlags::Save, "If non-negative, all LOD components use this LOD index (0 = highest detail), disabling the automatic selection.");

static float CalculateSphereScreenSpaceCoverage(const WBoundingSphere& sphere, const WCamera& camera)
{
  if (camera.IsPerspective())
  {
    return WGraphicsUtils::CalculateSphereScreenCoverage(sphere, camera.GetCenterPosition(), camera.GetFovY(1.0f));
  }
  else
  {
    return WGraphicsUtils::CalculateSphereScreenCoverage(sphere.m_fRadius, camera.GetDimensionY(1.0f));
  }
}

struct LodCompFlags
{
  enum Enum
  {
    ShowDebugInfo = 0,
    OverlapRanges = 1,
  };
};

// clang-format off
W_BEGIN_COMPONENT_TYPE(WLodComponent, 1, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("BoundsOffset", m_vBoundsOffset),
    W_MEMBER_PROPERTY("BoundsRadius", m_fBoundsRadius)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.01f, 100.0f)),
    W_ACCESSOR_PROPERTY("ShowDebugInfo", GetShowDebugInfo, SetShowDebugInfo),
    W_ACCESSOR_PROPERTY("OverlapRanges", GetOverlapRanges, SetOverlapRanges)->AddAttributes(new WDefaultValueAttribute(true)),
    W_ARRAY_MEMBER_PROPERTY("LodThresholds", m_LodThresholds)->AddAttributes(new WMaxArraySizeAttribute(4), new WClampValueAttribute(0.0f, 1.0f)),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Construction"),
    new WSphereVisualizerAttribute("BoundsRadius", WColor::MediumVioletRed, nullptr, WVisualizerAnchor::Center, WVec3(1.0f), "BoundsOffset"),
    new WTransformManipulatorAttribute("BoundsOffset"),
  }
  W_END_ATTRIBUTES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgExtractRenderData, OnMsgExtractRenderData),
    W_MESSAGE_HANDLER(WMsgComponentInternalTrigger, OnMsgComponentInternalTrigger),
  }
  W_END_MESSAGEHANDLERS;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

static const WTempHashedString sLod0("LOD0");
static const WTempHashedString sLod1("LOD1");
static const WTempHashedString sLod2("LOD2");
static const WTempHashedString sLod3("LOD3");
static const WTempHashedString sLod4("LOD4");

WLodComponent::WLodComponent()
{
  SetOverlapRanges(true);
}

WLodComponent::~WLodComponent() = default;

void WLodComponent::SetShowDebugInfo(bool bShow)
{
  SetUserFlag(LodCompFlags::ShowDebugInfo, bShow);
}

bool WLodComponent::GetShowDebugInfo() const
{
  return GetUserFlag(LodCompFlags::ShowDebugInfo);
}

void WLodComponent::SetOverlapRanges(bool bShow)
{
  SetUserFlag(LodCompFlags::OverlapRanges, bShow);
}

bool WLodComponent::GetOverlapRanges() const
{
  return GetUserFlag(LodCompFlags::OverlapRanges);
}

void WLodComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_vBoundsOffset;
  s << m_fBoundsRadius;

  s.WriteArray(m_LodThresholds).AssertSuccess();
}

void WLodComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s >> m_vBoundsOffset;
  s >> m_fBoundsRadius;

  s.ReadArray(m_LodThresholds).AssertSuccess();
}

WResult WLodComponent::GetLocalBounds(WBoundingBoxSphere& out_bounds, bool& out_bAlwaysVisible, WMsgUpdateLocalBounds& ref_msg)
{
  out_bounds = WBoundingSphere::MakeFromCenterAndRadius(m_vBoundsOffset, m_fBoundsRadius);
  out_bAlwaysVisible = false;
  return W_SUCCESS;
}

void WLodComponent::OnActivated()
{
  SUPER::OnActivated();

  // start with the highest LOD (lowest detail)
  m_iCurLod = m_LodThresholds.GetCount();

  WMsgComponentInternalTrigger trig;
  trig.m_iPayload = m_iCurLod;
  OnMsgComponentInternalTrigger(trig);
}

void WLodComponent::OnDeactivated()
{
  // when the component gets deactivated, activate all LOD children
  // this is important for editing to not behave weirdly
  // not sure whether this can have unintended side-effects at runtime
  // but there this should only be called for objects that get deleted anyway

  WGameObject* pLod[5];
  pLod[0] = GetOwner()->FindChildByName(sLod0);
  pLod[1] = GetOwner()->FindChildByName(sLod1);
  pLod[2] = GetOwner()->FindChildByName(sLod2);
  pLod[3] = GetOwner()->FindChildByName(sLod3);
  pLod[4] = GetOwner()->FindChildByName(sLod4);

  for (WUInt32 i = 0; i < W_ARRAY_SIZE(pLod); ++i)
  {
    if (pLod[i])
    {
      pLod[i]->SetActiveFlag(true);
    }
  }

  SUPER::OnDeactivated();
}

void WLodComponent::OnMsgExtractRenderData(WMsgExtractRenderData& msg) const
{
  if (msg.m_pView->GetCameraUsageHint() != WCameraUsageHint::EditorView &&
      msg.m_pView->GetCameraUsageHint() != WCameraUsageHint::MainView)
  {
    return;
  }

  // Don't extract render data for selection.
  if (msg.m_OverrideCategory != WInvalidRenderDataCategory)
    return;

  const WInt32 iNumLods = (WInt32)m_LodThresholds.GetCount();

  const WVec3 vScale = GetOwner()->GetGlobalScaling();
  const float fScale = WMath::Max(vScale.x, vScale.y, vScale.z);
  const WVec3 vCenter = GetOwner()->GetGlobalTransform() * m_vBoundsOffset;

  const float fCoverage = CalculateSphereScreenSpaceCoverage(WBoundingSphere::MakeFromCenterAndRadius(vCenter, fScale * m_fBoundsRadius), *msg.m_pView->GetCullingCamera()) * WMath::Max(0.0f, (float)cvar_RenderingLodCoverageScale);

  // clamp the input value, this is to prevent issues while editing the threshold array
  WInt32 iNewLod = WMath::Clamp<WInt32>(m_iCurLod, 0, iNumLods);

  float fCoverageP = 1;
  float fCoverageN = 0;

  if (iNewLod > 0)
  {
    fCoverageP = m_LodThresholds[iNewLod - 1];
  }

  if (iNewLod < iNumLods)
  {
    fCoverageN = m_LodThresholds[iNewLod];
  }

  if (GetOverlapRanges())
  {
    const float fLodRangeOverlap = 0.40f;

    if (iNewLod + 1 < iNumLods)
    {
      float range = (fCoverageN - m_LodThresholds[iNewLod + 1]);
      fCoverageN -= range * fLodRangeOverlap; // overlap into the next range
    }
    else
    {
      float range = (fCoverageN - 0.0f);
      fCoverageN -= range * fLodRangeOverlap; // overlap into the next range
    }
  }

  if (fCoverage < fCoverageN)
  {
    ++iNewLod;
  }
  else if (fCoverage > fCoverageP)
  {
    --iNewLod;
  }

  iNewLod = WMath::Clamp(iNewLod, 0, iNumLods);

  if (cvar_RenderingLodForce >= 0)
  {
    iNewLod = WMath::Min<WInt32>(cvar_RenderingLodForce, iNumLods);
  }

  if (GetShowDebugInfo())
  {
    WStringBuilder sb;
    sb.SetFormat("Coverage: {}\nLOD {}\nRange: {} - {}", WArgF(fCoverage, 3), iNewLod, WArgF(fCoverageP, 3), WArgF(fCoverageN, 3));
    WDebugRenderer::Draw3DText(msg.m_pView->GetHandle(), sb, GetOwner()->GetGlobalPosition(), WColor::White);
  }

  if (iNewLod == m_iCurLod)
    return;

  WMsgComponentInternalTrigger trig;
  trig.m_iPayload = iNewLod;

  PostMessage(trig);
}

void WLodComponent::OnMsgComponentInternalTrigger(WMsgComponentInternalTrigger& msg)
{
  m_iCurLod = msg.m_iPayload;

  // search for direct children named LODn, don't waste performance searching recursively
  WGameObject* pLod[5];
  pLod[0] = GetOwner()->FindChildByName(sLod0, false);
  pLod[1] = GetOwner()->FindChildByName(sLod1, false);
  pLod[2] = GetOwner()->FindChildByName(sLod2, false);
  pLod[3] = GetOwner()->FindChildByName(sLod3, false);
  pLod[4] = GetOwner()->FindChildByName(sLod4, false);

  // activate the selected LOD, deactivate all others
  for (WUInt32 i = 0; i < W_ARRAY_SIZE(pLod); ++i)
  {
    if (pLod[i])
    {
      pLod[i]->SetActiveFlag(m_iCurLod == i);
    }
  }
}


W_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_LodComponent);
