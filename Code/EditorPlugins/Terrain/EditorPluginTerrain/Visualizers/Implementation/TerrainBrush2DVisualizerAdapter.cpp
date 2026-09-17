#include <EditorPluginTerrain/EditorPluginTerrainPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorPluginTerrain/Visualizers/TerrainBrush2DVisualizerAdapter.h>
#include <TerrainPlugin/Components/TerrainBrushAttributes.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

WTerrainBrush2DVisualizerAdapter::WTerrainBrush2DVisualizerAdapter() = default;

WTerrainBrush2DVisualizerAdapter::~WTerrainBrush2DVisualizerAdapter() = default;

void WTerrainBrush2DVisualizerAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();
  const WAssetDocument* pAssetDocument = WDynamicCast<const WAssetDocument*>(pDoc);
  W_ASSERT_DEV(pAssetDocument != nullptr, "Visualizers are only supported in WAssetDocument.");

  const WTerrainBrush2DVisualizerAttribute* pAttr = static_cast<const WTerrainBrush2DVisualizerAttribute*>(m_pVisualizerAttr);

  m_hLinesInner.ConfigureHandle(nullptr, WEngineGizmoHandleType::CustomLines, pAttr->m_InnerColor, WGizmoFlags::ShowInOrtho | WGizmoFlags::Visualizer);
  m_hLinesOuter.ConfigureHandle(nullptr, WEngineGizmoHandleType::CustomLines, pAttr->m_OuterColor, WGizmoFlags::ShowInOrtho | WGizmoFlags::Visualizer);

  pAssetDocument->AddSyncObject(&m_hLinesInner);
  pAssetDocument->AddSyncObject(&m_hLinesOuter);
}

// Draws a rounded rectangle in the XY plane at the given Z offset.
void BuildRoundedRectAtZ(WDynamicArray<WVec3>& ref_lines, float fHalfX, float fHalfY, float fRadius, float fZ)
{
  if (fHalfX > 0.0f)
  {
    ref_lines.PushBack(WVec3(-fHalfX, -(fHalfY + fRadius), fZ));
    ref_lines.PushBack(WVec3(+fHalfX, -(fHalfY + fRadius), fZ));

    ref_lines.PushBack(WVec3(+fHalfX, +(fHalfY + fRadius), fZ));
    ref_lines.PushBack(WVec3(-fHalfX, +(fHalfY + fRadius), fZ));
  }
  if (fHalfY > 0.0f)
  {
    ref_lines.PushBack(WVec3(+(fHalfX + fRadius), -fHalfY, fZ));
    ref_lines.PushBack(WVec3(+(fHalfX + fRadius), +fHalfY, fZ));

    ref_lines.PushBack(WVec3(-(fHalfX + fRadius), +fHalfY, fZ));
    ref_lines.PushBack(WVec3(-(fHalfX + fRadius), -fHalfY, fZ));
  }

  if (fRadius <= 0.0f)
    return;

  constexpr WUInt32 uiSegmentsPerQuarter = 8;
  constexpr float fStep = WMath::Pi<float>() * 0.5f / uiSegmentsPerQuarter;

  const WVec2 vCorners[4] = {
    WVec2(+fHalfX, -fHalfY),
    WVec2(+fHalfX, +fHalfY),
    WVec2(-fHalfX, +fHalfY),
    WVec2(-fHalfX, -fHalfY),
  };
  const float fStartAngles[4] = {
    -WMath::Pi<float>() * 0.5f,
    0.0f,
    WMath::Pi<float>() * 0.5f,
    WMath::Pi<float>(),
  };

  for (WUInt32 c = 0; c < 4; ++c)
  {
    for (WUInt32 s = 0; s < uiSegmentsPerQuarter; ++s)
    {
      const WAngle fA0 = WAngle::MakeFromRadian(fStartAngles[c] + s * fStep);
      const WAngle fA1 = WAngle::MakeFromRadian(fStartAngles[c] + (s + 1) * fStep);

      ref_lines.PushBack(WVec3(vCorners[c].x + fRadius * WMath::Cos(fA0), vCorners[c].y + fRadius * WMath::Sin(fA0), fZ));
      ref_lines.PushBack(WVec3(vCorners[c].x + fRadius * WMath::Cos(fA1), vCorners[c].y + fRadius * WMath::Sin(fA1), fZ));
    }
  }
}

static void BuildRoundedRectLines(WDynamicArray<WVec3>& ref_lines, float fHalfX, float fHalfY, float fRadius)
{
  ref_lines.Clear();
  BuildRoundedRectAtZ(ref_lines, fHalfX, fHalfY, fRadius, 0.0f);
}

void WTerrainBrush2DVisualizerAdapter::Update()
{
  const WTerrainBrush2DVisualizerAttribute* pAttr = static_cast<const WTerrainBrush2DVisualizerAttribute*>(m_pVisualizerAttr);
  WObjectAccessorBase* pObjectAccessor = GetObjectAccessor();

  float fSizeX = 0;
  float fSizeY = 0;
  float fInnerRadius = 0;
  float fOuterRadius = 0;

  if (!pAttr->GetPropHalfSizeX().IsEmpty())
  {
    WVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetPropHalfSizeX()), value).AssertSuccess();

    W_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<float>(), "Invalid property bound to WTerrainBrush2DVisualizerAttribute 'size x'");
    fSizeX = value.ConvertTo<float>();
  }

  if (!pAttr->GetPropHalfSizeY().IsEmpty())
  {
    WVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetPropHalfSizeY()), value).AssertSuccess();

    W_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<float>(), "Invalid property bound to WTerrainBrush2DVisualizerAttribute 'size y'");
    fSizeY = value.ConvertTo<float>();
  }

  if (!pAttr->GetPropInnerRadius().IsEmpty())
  {
    WVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetPropInnerRadius()), value).AssertSuccess();

    W_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<float>(), "Invalid property bound to WTerrainBrush2DVisualizerAttribute 'inner'");
    fInnerRadius = value.ConvertTo<float>();
  }

  if (!pAttr->GetPropOuterRadius().IsEmpty())
  {
    WVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetPropOuterRadius()), value).AssertSuccess();

    W_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<float>(), "Invalid property bound to WTerrainBrush2DVisualizerAttribute 'outer'");
    fOuterRadius = value.ConvertTo<float>();
  }

  WTempHybridArray<WVec3, 64> ref_lines;

  if (fSizeX > 0 || fSizeY > 0 || fInnerRadius > 0)
  {
    BuildRoundedRectLines(ref_lines, fSizeX, fSizeY, fInnerRadius);
    m_hLinesInner.SetLines(ref_lines);
    m_hLinesInner.SetVisible(m_bVisualizerIsVisible);
  }
  else
  {
    m_hLinesInner.SetVisible(false);
  }

  if (fOuterRadius > 0)
  {
    BuildRoundedRectLines(ref_lines, fSizeX, fSizeY, fInnerRadius + fOuterRadius);
    m_hLinesOuter.SetLines(ref_lines);
    m_hLinesOuter.SetVisible(m_bVisualizerIsVisible);
  }
  else
  {
    m_hLinesOuter.SetVisible(false);
  }
}

void WTerrainBrush2DVisualizerAdapter::UpdateGizmoTransform()
{
  const WTerrainBrush2DVisualizerAttribute* pAttr = static_cast<const WTerrainBrush2DVisualizerAttribute*>(m_pVisualizerAttr);

  WTransform t = GetObjectTransform();
  t.m_vPosition += t.m_qRotation * pAttr->m_vOffset;

  m_hLinesInner.SetTransformation(t);
  m_hLinesOuter.SetTransformation(t);
}
