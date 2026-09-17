#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <Core/Messages/ApplyOnlyToMessage.h>
#include <Core/Messages/DeleteObjectMessage.h>
#include <Core/Messages/SetColorMessage.h>
#include <Core/Messages/TriggerMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <RendererCore/Decals/DecalAtlasResource.h>
#include <RendererCore/Decals/DecalComponent.h>
#include <RendererCore/Decals/DecalResource.h>
#include <RendererCore/Decals/Implementation/DecalManager.h>
#include <RendererCore/Pipeline/RenderDataManager.h>
#include <RendererFoundation/Shader/ShaderUtils.h>

#include <RendererCore/../../../Data/Base/Shaders/Common/LightData.h>

//////////////////////////////////////////////////////////////////////////

namespace
{
  // The number of grid cells that an atlas item is subdivided into. At least 1x1.
  WVec2U32 GetVariationGridSize(const WTextureAtlasRuntimeDesc::Item& item)
  {
    return WVec2U32(WMath::Max<WUInt32>(1, item.m_uiNumVariationsX), WMath::Max<WUInt32>(1, item.m_uiNumVariationsY));
  }

  // The column and row of the cell to display.
  //
  // uiVariation is the one-based index that the user selected, zero means to use uiRandomIdx instead.
  // Indices that exceed the number of cells wrap around.
  WVec2U32 GetVariationCell(const WVec2U32& vGridSize, WInt8 iVariation, WUInt8 uiRandomIdx)
  {
    const WUInt32 uiIdx = (iVariation < 0 ? uiRandomIdx : (WUInt32)iVariation) % (vGridSize.x * vGridSize.y);
    return WVec2U32(uiIdx % vGridSize.x, uiIdx / vGridSize.x);
  }

  float GetCellAspectRatio(const WRectU32& layerRect, const WVec2U32& vGridSize)
  {
    return ((float)layerRect.width / vGridSize.x) / ((float)layerRect.height / vGridSize.y);
  }

  // Converts the cell of an atlas item into the scale/offset pair that the decal shader uses to map
  // the decal's local position (in [-1;1]) to atlas UVs.
  WVec4 LayerRectToScaleOffset(const WRectU32& layerRect, const WVec2U32& vTextureSize, const WVec2U32& vGridSize, const WVec2U32& vCell)
  {
    const float fCellWidth = (float)layerRect.width / vGridSize.x;
    const float fCellHeight = (float)layerRect.height / vGridSize.y;

    WVec4 result;
    result.x = fCellWidth / vTextureSize.x * 0.5f;
    result.y = fCellHeight / vTextureSize.y * 0.5f;
    result.z = ((float)layerRect.x + vCell.x * fCellWidth) / vTextureSize.x + result.x;
    result.w = ((float)layerRect.y + vCell.y * fCellHeight) / vTextureSize.y + result.y;
    return result;
  }
} // namespace

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WDecalRenderData, 1, WRTTIDefaultAllocator<WDecalRenderData>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_COMPONENT_TYPE(WDecalComponent, 9, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_ARRAY_ACCESSOR_PROPERTY("Decals", DecalFile_GetCount, DecalFile_Get, DecalFile_Set, DecalFile_Insert, DecalFile_Remove)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Decal"), new WRequiredAttribute()),
    W_ENUM_ACCESSOR_PROPERTY("ProjectionAxis", WBasisAxis, GetProjectionAxis, SetProjectionAxis),
    W_ACCESSOR_PROPERTY("Extents", GetExtents, SetExtents)->AddAttributes(new WDefaultValueAttribute(WVec3(1.0f)), new WClampValueAttribute(WVec3(0.01f), WVariant(25.0f))),
    W_ACCESSOR_PROPERTY("SizeVariance", GetSizeVariance, SetSizeVariance)->AddAttributes(new WClampValueAttribute(0.0f, 1.0f)),
    W_ACCESSOR_PROPERTY("Color", GetColor, SetColor)->AddAttributes(new WExposeColorAlphaAttribute()),
    W_ACCESSOR_PROPERTY("EmissiveColor", GetEmissiveColor, SetEmissiveColor)->AddAttributes(new WDefaultValueAttribute(WColor::Black)),
    W_ACCESSOR_PROPERTY("SortOrder", GetSortOrder, SetSortOrder)->AddAttributes(new WClampValueAttribute(-64.0f, 64.0f)),
    W_ACCESSOR_PROPERTY("WrapAround", GetWrapAround, SetWrapAround),
    W_ACCESSOR_PROPERTY("MapNormalToGeometry", GetMapNormalToGeometry, SetMapNormalToGeometry)->AddAttributes(new WDefaultValueAttribute(true)),
    W_ACCESSOR_PROPERTY("InnerFadeAngle", GetInnerFadeAngle, SetInnerFadeAngle)->AddAttributes(new WClampValueAttribute(WAngle::MakeFromDegree(0.0f), WAngle::MakeFromDegree(89.0f)), new WDefaultValueAttribute(WAngle::MakeFromDegree(50.0f))),
    W_ACCESSOR_PROPERTY("OuterFadeAngle", GetOuterFadeAngle, SetOuterFadeAngle)->AddAttributes(new WClampValueAttribute(WAngle::MakeFromDegree(0.0f), WAngle::MakeFromDegree(89.0f)), new WDefaultValueAttribute(WAngle::MakeFromDegree(80.0f))),
    W_MEMBER_PROPERTY("FadeOutDelay", m_FadeOutDelay),
    W_MEMBER_PROPERTY("FadeOutDuration", m_FadeOutDuration),
    W_ENUM_MEMBER_PROPERTY("OnFinishedAction", WOnComponentFinishedAction, m_OnFinishedAction),
    W_ACCESSOR_PROPERTY("ApplyToDynamic", DummyGetter, SetApplyToRef)->AddAttributes(new WGameObjectReferenceAttribute()),
    W_ACCESSOR_PROPERTY("Variation", GetVariation, SetVariation)->AddAttributes(new WDefaultValueAttribute(-1), new WClampValueAttribute(-1, 255), new WMinValueTextAttribute("Auto")),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Effects"),
    new WDirectionVisualizerAttribute("ProjectionAxis", 0.5f, WColorScheme::LightUI(WColorScheme::Blue)),
    new WBoxManipulatorAttribute("Extents", 1.0f, true),
    new WBoxVisualizerAttribute("Extents"),
  }
  W_END_ATTRIBUTES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgExtractRenderData, OnMsgExtractRenderData),
    W_MESSAGE_HANDLER(WMsgComponentInternalTrigger, OnTriggered),
    W_MESSAGE_HANDLER(WMsgDeleteGameObject, OnMsgDeleteGameObject),
    W_MESSAGE_HANDLER(WMsgOnlyApplyToObject, OnMsgOnlyApplyToObject),
    W_MESSAGE_HANDLER(WMsgSetColor, OnMsgSetColor),
  }
  W_END_MESSAGEHANDLERS;
}
W_END_COMPONENT_TYPE
// clang-format on

WDecalComponent::WDecalComponent() = default;

WDecalComponent::~WDecalComponent() = default;

void WDecalComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  WStreamWriter& s = inout_stream.GetStream();

  s << m_vExtents;
  s << m_Color;
  s << m_EmissiveColor;
  s << m_InnerFadeAngle;
  s << m_OuterFadeAngle;
  s << m_fSortOrder;
  s << m_FadeOutDelay.m_Value;
  s << m_FadeOutDelay.m_fVariance;
  s << m_FadeOutDuration;
  s << m_StartFadeOutTime;
  s << m_fSizeVariance;
  s << m_OnFinishedAction;
  s << m_bWrapAround;
  s << m_bMapNormalToGeometry;

  // version 5
  s << m_ProjectionAxis;

  // version 6
  inout_stream.WriteGameObjectHandle(m_hApplyOnlyToObject);

  // version 7
  s << m_uiRandomDecalIdx;
  s.WriteArray(m_Decals).IgnoreResult();

  // version 9
  s << m_iVariation;
  s << m_uiRandomVariationIdx;
}

void WDecalComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  WStreamReader& s = inout_stream.GetStream();

  s >> m_vExtents;

  if (uiVersion >= 4)
  {
    s >> m_Color;
    s >> m_EmissiveColor;
  }
  else
  {
    WColor tmp;
    s >> tmp;
    m_Color = tmp;
  }

  s >> m_InnerFadeAngle;
  s >> m_OuterFadeAngle;
  s >> m_fSortOrder;

  if (uiVersion <= 7)
  {
    WUInt32 dummy;
    s >> dummy;
  }

  m_uiInternalSortKey = GetOwner()->GetStableRandomSeed();
  m_uiInternalSortKey = (m_uiInternalSortKey >> 16) ^ (m_uiInternalSortKey & 0xFFFF);

  if (uiVersion < 7)
  {
    m_Decals.SetCount(1);
    s >> m_Decals[0];
  }

  s >> m_FadeOutDelay.m_Value;
  s >> m_FadeOutDelay.m_fVariance;
  s >> m_FadeOutDuration;
  s >> m_StartFadeOutTime;
  s >> m_fSizeVariance;
  s >> m_OnFinishedAction;

  if (uiVersion >= 3)
  {
    s >> m_bWrapAround;
  }

  if (uiVersion >= 4)
  {
    s >> m_bMapNormalToGeometry;
  }

  if (uiVersion >= 5)
  {
    s >> m_ProjectionAxis;
  }

  if (uiVersion >= 6)
  {
    SetApplyOnlyTo(inout_stream.ReadGameObjectHandle());
  }

  if (uiVersion >= 7)
  {
    s >> m_uiRandomDecalIdx;
    s.ReadArray(m_Decals).IgnoreResult();
  }

  if (uiVersion >= 9)
  {
    s >> m_iVariation;
    s >> m_uiRandomVariationIdx;
  }
}

WResult WDecalComponent::GetLocalBounds(WBoundingBoxSphere& bounds, bool& bAlwaysVisible, WMsgUpdateLocalBounds& msg)
{
  if (m_Decals.IsEmpty())
    return W_FAILURE;

  const WUInt32 uiStableSeed = GetOwner()->GetStableRandomSeed();
  m_uiRandomDecalIdx = (uiStableSeed % m_Decals.GetCount()) & 0xFF;

  // hash the seed again, so that the chosen variation doesn't correlate with the chosen decal
  m_uiRandomVariationIdx = static_cast<WUInt8>(WHashHelper<WUInt32>::Hash(uiStableSeed) & 0xFF);

  const WUInt32 uiDecalIndex = WMath::Min<WUInt32>(m_uiRandomDecalIdx, m_Decals.GetCount() - 1);

  if (!m_Decals[uiDecalIndex].IsValid() || m_vExtents.IsZero())
    return W_FAILURE;

  float fAspectRatio = 1.0f;

  {
    auto hDecalAtlas = WDecalManager::GetBakedDecalAtlas();
    WResourceLock<WDecalAtlasResource> pDecalAtlas(hDecalAtlas, WResourceAcquireMode::BlockTillLoaded);

    const auto& atlas = pDecalAtlas->GetAtlas();
    const WUInt32 decalIdx = atlas.m_Items.Find(WHashingUtils::StringHashTo32(m_Decals[uiDecalIndex].GetResourceIDHash()));

    if (decalIdx != WInvalidIndex)
    {
      const auto& item = atlas.m_Items.GetValue(decalIdx);
      const WUInt32 uiLayerIdx = item.m_LayerRects[0].width > 0 ? 0 : 2;
      fAspectRatio = GetCellAspectRatio(item.m_LayerRects[uiLayerIdx], GetVariationGridSize(item));
    }
  }

  WVec3 vAspectCorrection = WVec3(1.0f);
  if (!WMath::IsEqual(fAspectRatio, 1.0f, 0.001f))
  {
    if (fAspectRatio > 1.0f)
    {
      vAspectCorrection.z /= fAspectRatio;
    }
    else
    {
      vAspectCorrection.y *= fAspectRatio;
    }
  }

  const WQuat axisRotation = WBasisAxis::GetBasisRotation_PosX(m_ProjectionAxis);
  WVec3 vHalfExtents = (axisRotation * vAspectCorrection).Abs().CompMul(m_vExtents * 0.5f);

  bounds = WBoundingBoxSphere::MakeFromBox(WBoundingBox::MakeFromMinMax(-vHalfExtents, vHalfExtents));
  return W_SUCCESS;
}

void WDecalComponent::SetExtents(const WVec3& value)
{
  m_vExtents = value.CompMax(WVec3::MakeZero());

  TriggerLocalBoundsUpdate();
}

const WVec3& WDecalComponent::GetExtents() const
{
  return m_vExtents;
}

void WDecalComponent::SetSizeVariance(float fVariance)
{
  m_fSizeVariance = WMath::Clamp(fVariance, 0.0f, 1.0f);
}

float WDecalComponent::GetSizeVariance() const
{
  return m_fSizeVariance;
}

void WDecalComponent::SetColor(WColorGammaUB color)
{
  m_Color = color;
}

WColorGammaUB WDecalComponent::GetColor() const
{
  return m_Color;
}

void WDecalComponent::SetEmissiveColor(WColor color)
{
  m_EmissiveColor = color;
}

WColor WDecalComponent::GetEmissiveColor() const
{
  return m_EmissiveColor;
}

void WDecalComponent::SetInnerFadeAngle(WAngle spotAngle)
{
  m_InnerFadeAngle = WMath::Clamp(spotAngle, WAngle::MakeFromDegree(0.0f), m_OuterFadeAngle);
}

WAngle WDecalComponent::GetInnerFadeAngle() const
{
  return m_InnerFadeAngle;
}

void WDecalComponent::SetOuterFadeAngle(WAngle spotAngle)
{
  m_OuterFadeAngle = WMath::Clamp(spotAngle, m_InnerFadeAngle, WAngle::MakeFromDegree(90.0f));
}

WAngle WDecalComponent::GetOuterFadeAngle() const
{
  return m_OuterFadeAngle;
}

void WDecalComponent::SetSortOrder(float fOrder)
{
  m_fSortOrder = fOrder;
}

float WDecalComponent::GetSortOrder() const
{
  return m_fSortOrder;
}

void WDecalComponent::SetWrapAround(bool bWrapAround)
{
  m_bWrapAround = bWrapAround;
}

bool WDecalComponent::GetWrapAround() const
{
  return m_bWrapAround;
}

void WDecalComponent::SetMapNormalToGeometry(bool bMapNormal)
{
  m_bMapNormalToGeometry = bMapNormal;
}

bool WDecalComponent::GetMapNormalToGeometry() const
{
  return m_bMapNormalToGeometry;
}

void WDecalComponent::SetDecal(WUInt32 uiIndex, const WDecalResourceHandle& hDecal)
{
  m_Decals.EnsureCount(uiIndex + 1);
  m_Decals[uiIndex] = hDecal;

  TriggerLocalBoundsUpdate();
}

const WDecalResourceHandle& WDecalComponent::GetDecal(WUInt32 uiIndex) const
{
  return m_Decals[uiIndex];
}

void WDecalComponent::SetVariation(WInt8 iVariation)
{
  m_iVariation = iVariation;

  // all cells have the same size, so the bounds don't change, but the atlas UVs do
  InvalidateCachedRenderData();
}

WInt8 WDecalComponent::GetVariation() const
{
  return m_iVariation;
}

void WDecalComponent::SetProjectionAxis(WEnum<WBasisAxis> projectionAxis)
{
  m_ProjectionAxis = projectionAxis;

  TriggerLocalBoundsUpdate();
}

WEnum<WBasisAxis> WDecalComponent::GetProjectionAxis() const
{
  return m_ProjectionAxis;
}

void WDecalComponent::SetApplyOnlyTo(WGameObjectHandle hObject)
{
  if (m_hApplyOnlyToObject != hObject)
  {
    m_hApplyOnlyToObject = hObject;
    UpdateApplyTo();
  }
}

WGameObjectHandle WDecalComponent::GetApplyOnlyTo() const
{
  return m_hApplyOnlyToObject;
}

void WDecalComponent::OnMsgExtractRenderData(WMsgExtractRenderData& msg) const
{
  // Don't extract decal render data for selection.
  if (msg.m_OverrideCategory != WInvalidRenderDataCategory)
    return;

  if (m_Decals.IsEmpty())
    return;

  const WUInt32 uiDecalIndex = WMath::Min<WUInt32>(m_uiRandomDecalIdx, m_Decals.GetCount() - 1);

  if (!m_Decals[uiDecalIndex].IsValid() || m_vExtents.IsZero() || GetOwner()->GetLocalScaling().IsZero())
    return;

  float fFade = 1.0f;

  const WTime tNow = GetWorld()->GetClock().GetAccumulatedTime();
  if (tNow > m_StartFadeOutTime)
  {
    fFade -= WMath::Min<float>(1.0f, (float)((tNow - m_StartFadeOutTime).GetSeconds() / m_FadeOutDuration.GetSeconds()));
  }

  WColor finalColor = m_Color;
  finalColor.a *= fFade;

  if (finalColor.a <= 0.0f)
    return;

  const bool bNoFade = m_InnerFadeAngle == WAngle::MakeFromRadian(0.0f) && m_OuterFadeAngle == WAngle::MakeFromRadian(0.0f);
  const float fCosInner = WMath::Cos(m_InnerFadeAngle);
  const float fCosOuter = WMath::Cos(m_OuterFadeAngle);
  const float fFadeParamScale = bNoFade ? 0.0f : (1.0f / WMath::Max(0.001f, (fCosInner - fCosOuter)));
  const float fFadeParamOffset = bNoFade ? 1.0f : (-fCosOuter * fFadeParamScale);

  auto hDecalAtlas = WDecalManager::GetBakedDecalAtlas();
  WVec4 baseAtlasScaleOffset = WVec4(0.5f);
  WVec4 normalAtlasScaleOffset = WVec4(0.5f);
  WVec4 ormAtlasScaleOffset = WVec4(0.5f);
  WUInt32 uiDecalFlags = 0;

  float fAspectRatio = 1.0f;

  {
    WResourceLock<WDecalAtlasResource> pDecalAtlas(hDecalAtlas, WResourceAcquireMode::BlockTillLoaded);

    const auto& atlas = pDecalAtlas->GetAtlas();
    const WUInt32 decalIdx = atlas.m_Items.Find(WHashingUtils::StringHashTo32(m_Decals[uiDecalIndex].GetResourceIDHash()));

    if (decalIdx != WInvalidIndex)
    {
      const auto& item = atlas.m_Items.GetValue(decalIdx);
      uiDecalFlags = item.m_uiFlags;

      const WVec2U32 vGridSize = GetVariationGridSize(item);
      const WVec2U32 vCell = GetVariationCell(vGridSize, m_iVariation, m_uiRandomVariationIdx);

      baseAtlasScaleOffset = LayerRectToScaleOffset(item.m_LayerRects[0], pDecalAtlas->GetBaseColorTextureSize(), vGridSize, vCell);
      normalAtlasScaleOffset = LayerRectToScaleOffset(item.m_LayerRects[1], pDecalAtlas->GetNormalTextureSize(), vGridSize, vCell);
      ormAtlasScaleOffset = LayerRectToScaleOffset(item.m_LayerRects[2], pDecalAtlas->GetORMTextureSize(), vGridSize, vCell);

      const WUInt32 uiLayerIdx = item.m_LayerRects[0].width > 0 ? 0 : 2;
      fAspectRatio = GetCellAspectRatio(item.m_LayerRects[uiLayerIdx], vGridSize);
    }
  }

  auto pRenderData = msg.m_pRenderDataManager->CreateRenderDataForThisFrame<WDecalRenderData>(GetOwner());

  WUInt32 uiSortingId = (WUInt32)(WMath::Min(m_fSortOrder * 512.0f, 32767.0f) + 32768.0f);
  pRenderData->m_uiSortingKey = (uiSortingId << 16) | (m_uiInternalSortKey & 0xFFFF);

  const WQuat axisRotation = WBasisAxis::GetBasisRotation_PosX(m_ProjectionAxis);
  const WTransform globalTransform = GetOwner()->GetGlobalTransform();

  const WQuat finalRotation = globalTransform.m_qRotation * axisRotation;
  pRenderData->m_qGlobalRotation = WVec4(finalRotation.x, finalRotation.y, finalRotation.z, finalRotation.w);

  WVec3 finalScale = (axisRotation * (globalTransform.m_vScale.CompMul(m_vExtents * 0.5f))).Abs();
  if (!WMath::IsEqual(fAspectRatio, 1.0f, 0.001f))
  {
    if (fAspectRatio > 1.0f)
    {
      finalScale.z /= fAspectRatio;
    }
    else
    {
      finalScale.y *= fAspectRatio;
    }
  }
  pRenderData->m_vGlobalScale = finalScale;

  pRenderData->m_uiApplyOnlyToId = m_uiApplyOnlyToId;
  pRenderData->m_uiFlags = uiDecalFlags;
  pRenderData->m_uiFlags |= (m_bWrapAround ? DECAL_WRAP_AROUND : 0);
  pRenderData->m_uiFlags |= (m_bMapNormalToGeometry ? DECAL_MAP_NORMAL_TO_GEOMETRY : 0);
  pRenderData->m_uiAngleFadeParams = WShaderUtils::Float2ToRG16F(WVec2(fFadeParamScale, fFadeParamOffset));
  pRenderData->m_BaseColor = finalColor;
  pRenderData->m_EmissiveColor = m_EmissiveColor;
  WShaderUtils::Float4ToRGBA16F(baseAtlasScaleOffset, pRenderData->m_uiBaseColorAtlasScale, pRenderData->m_uiBaseColorAtlasOffset);
  WShaderUtils::Float4ToRGBA16F(normalAtlasScaleOffset, pRenderData->m_uiNormalAtlasScale, pRenderData->m_uiNormalAtlasOffset);
  WShaderUtils::Float4ToRGBA16F(ormAtlasScaleOffset, pRenderData->m_uiORMAtlasScale, pRenderData->m_uiORMAtlasOffset);

  WRenderData::Caching::Enum caching = (m_FadeOutDelay.m_Value.GetSeconds() > 0.0 || m_FadeOutDuration.GetSeconds() > 0.0) ? WRenderData::Caching::Never : WRenderData::Caching::IfStatic;
  msg.AddRenderData(pRenderData, WDefaultRenderDataCategories::Decal, caching);
}

void WDecalComponent::SetApplyToRef(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  WGameObjectHandle hTarget = resolver(szReference, GetHandle(), "ApplyTo");

  if (m_hApplyOnlyToObject == hTarget)
    return;

  m_hApplyOnlyToObject = hTarget;

  if (IsActiveAndInitialized())
  {
    UpdateApplyTo();
  }
}

void WDecalComponent::UpdateApplyTo()
{
  WUInt32 uiPrevId = m_uiApplyOnlyToId;

  m_uiApplyOnlyToId = 0;

  if (!m_hApplyOnlyToObject.IsInvalidated())
  {
    m_uiApplyOnlyToId = WInvalidIndex;

    WGameObject* pObject = nullptr;
    if (GetWorld()->TryGetObject(m_hApplyOnlyToObject, pObject))
    {
      WRenderComponent* pRenderComponent = nullptr;
      if (pObject->TryGetComponentOfBaseType(pRenderComponent))
      {
        // this only works for dynamic objects, for static ones we must use ID 0
        if (pRenderComponent->GetOwner()->IsDynamic())
        {
          m_uiApplyOnlyToId = pRenderComponent->GetUniqueIdForRendering();
        }
      }
    }
  }

  if (uiPrevId != m_uiApplyOnlyToId && GetOwner()->IsStatic())
  {
    InvalidateCachedRenderData();
  }
}

static WHashedString s_sSuicide = WMakeHashedString("Suicide");

void WDecalComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  WWorld* pWorld = GetWorld();

  // no fade out -> fade out pretty late
  m_StartFadeOutTime = WTime::MakeFromHours(24.0 * 365.0 * 100.0); // 100 years should be enough for everybody (ignoring leap years)

  if (m_FadeOutDelay.m_Value.GetSeconds() > 0.0 || m_FadeOutDuration.GetSeconds() > 0.0)
  {
    const WTime tFadeOutDelay = WTime::MakeFromSeconds(pWorld->GetRandomNumberGenerator().DoubleVariance(m_FadeOutDelay.m_Value.GetSeconds(), m_FadeOutDelay.m_fVariance));
    m_StartFadeOutTime = pWorld->GetClock().GetAccumulatedTime() + tFadeOutDelay;

    if (m_OnFinishedAction != WOnComponentFinishedAction::None)
    {
      WMsgComponentInternalTrigger msg;
      msg.m_sMessage = s_sSuicide;

      const WTime tKill = tFadeOutDelay + m_FadeOutDuration;

      PostMessage(msg, tKill);
    }
  }

  if (m_fSizeVariance > 0)
  {
    const float scale = (float)pWorld->GetRandomNumberGenerator().DoubleVariance(1.0, m_fSizeVariance);
    m_vExtents *= scale;

    TriggerLocalBoundsUpdate();

    InvalidateCachedRenderData();
  }
}

void WDecalComponent::OnActivated()
{
  SUPER::OnActivated();

  m_uiInternalSortKey = GetOwner()->GetStableRandomSeed();
  m_uiInternalSortKey = (m_uiInternalSortKey >> 16) ^ (m_uiInternalSortKey & 0xFFFF);

  UpdateApplyTo();
}

void WDecalComponent::OnTriggered(WMsgComponentInternalTrigger& msg)
{
  if (msg.m_sMessage != s_sSuicide)
    return;

  WOnComponentFinishedAction::HandleFinishedAction(this, m_OnFinishedAction);
}

void WDecalComponent::OnMsgDeleteGameObject(WMsgDeleteGameObject& msg)
{
  WOnComponentFinishedAction::HandleDeleteObjectMsg(msg, m_OnFinishedAction);
}

void WDecalComponent::OnMsgOnlyApplyToObject(WMsgOnlyApplyToObject& msg)
{
  SetApplyOnlyTo(msg.m_hObject);
}

void WDecalComponent::OnMsgSetColor(WMsgSetColor& ref_msg)
{
  WColor newColor = m_Color;
  ref_msg.ModifyColor(newColor);

  if (m_Color != newColor)
  {
    m_Color = newColor;

    InvalidateCachedRenderData();
  }
}

WUInt32 WDecalComponent::DecalFile_GetCount() const
{
  return m_Decals.GetCount();
}

WString WDecalComponent::DecalFile_Get(WUInt32 uiIndex) const
{
  return m_Decals[uiIndex].GetResourceID();
}

void WDecalComponent::DecalFile_Set(WUInt32 uiIndex, WString sFile)
{
  WDecalResourceHandle hResource;

  if (!sFile.IsEmpty())
  {
    hResource = WResourceManager::LoadResource<WDecalResource>(sFile);
  }

  SetDecal(uiIndex, hResource);
}

void WDecalComponent::DecalFile_Insert(WUInt32 uiIndex, WString sFile)
{
  m_Decals.InsertAt(uiIndex, WDecalResourceHandle());
  DecalFile_Set(uiIndex, sFile);
}

void WDecalComponent::DecalFile_Remove(WUInt32 uiIndex)
{
  m_Decals.RemoveAtAndCopy(uiIndex);
}

//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class WDecalComponent_6_7 : public WGraphPatch
{
public:
  WDecalComponent_6_7()
    : WGraphPatch("WDecalComponent", 7)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    auto* pDecal = pNode->FindProperty("Decal");
    if (pDecal && pDecal->m_Value.IsA<WString>())
    {
      WVariantArray ar;
      ar.PushBack(pDecal->m_Value.Get<WString>());
      pNode->AddProperty("Decals", ar);
    }
  }
};

WDecalComponent_6_7 g_WDecalComponent_6_7;

W_STATICLINK_FILE(RendererCore, RendererCore_Decals_Implementation_DecalComponent);
