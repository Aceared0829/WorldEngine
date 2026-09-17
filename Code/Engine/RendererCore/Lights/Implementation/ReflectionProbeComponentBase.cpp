#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Lights/ReflectionProbeComponentBase.h>

#include <Core/Graphics/Camera.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/View.h>

namespace
{
  static WVariantArray GetDefaultExcludeTags()
  {
    WVariantArray value(WStaticsAllocatorWrapper::GetAllocator());
    value.PushBack(WStringView("SkyLight"));
    return value;
  }
} // namespace


// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WReflectionProbeComponentBase, 2, WRTTINoAllocator)
{
  W_BEGIN_PROPERTIES
  {
    W_ENUM_ACCESSOR_PROPERTY("ReflectionProbeMode", WReflectionProbeMode, GetReflectionProbeMode, SetReflectionProbeMode)->AddAttributes(new WDefaultValueAttribute(WReflectionProbeMode::Static), new WGroupAttribute("Capture Description")),
    W_SET_ACCESSOR_PROPERTY("IncludeTags", GetIncludeTags, InsertIncludeTag, RemoveIncludeTag)->AddAttributes(new WTagSetWidgetAttribute("Default")),
    W_SET_ACCESSOR_PROPERTY("ExcludeTags", GetExcludeTags, InsertExcludeTag, RemoveExcludeTag)->AddAttributes(new WTagSetWidgetAttribute("Default"), new WDefaultValueAttribute(GetDefaultExcludeTags())),
    W_ACCESSOR_PROPERTY("NearPlane", GetNearPlane, SetNearPlane)->AddAttributes(new WDefaultValueAttribute(0.0f), new WClampValueAttribute(0.0f, {}), new WMinValueTextAttribute("Auto")),
    W_ACCESSOR_PROPERTY("FarPlane", GetFarPlane, SetFarPlane)->AddAttributes(new WDefaultValueAttribute(100.0f), new WClampValueAttribute(0.01f, 10000.0f)),
    W_ACCESSOR_PROPERTY("CaptureOffset", GetCaptureOffset, SetCaptureOffset),
    W_ACCESSOR_PROPERTY("ShowDebugInfo", GetShowDebugInfo, SetShowDebugInfo),
    W_ACCESSOR_PROPERTY("ShowMipMaps", GetShowMipMaps, SetShowMipMaps),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Rendering/Reflections"),
    new WTransformManipulatorAttribute("CaptureOffset"),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WReflectionProbeComponentBase::WReflectionProbeComponentBase()
{
  m_Desc.m_uniqueID = WUuid::MakeUuid();
}

WReflectionProbeComponentBase::~WReflectionProbeComponentBase() = default;

void WReflectionProbeComponentBase::SetReflectionProbeMode(WEnum<WReflectionProbeMode> mode)
{
  m_Desc.m_Mode = mode;
  m_bStatesDirty = true;
}

WEnum<WReflectionProbeMode> WReflectionProbeComponentBase::GetReflectionProbeMode() const
{
  return m_Desc.m_Mode;
}

const WTagSet& WReflectionProbeComponentBase::GetIncludeTags() const
{
  return m_Desc.m_IncludeTags;
}

void WReflectionProbeComponentBase::InsertIncludeTag(const char* szTag)
{
  m_Desc.m_IncludeTags.SetByName(szTag);
  m_bStatesDirty = true;
}

void WReflectionProbeComponentBase::RemoveIncludeTag(const char* szTag)
{
  m_Desc.m_IncludeTags.RemoveByName(szTag);
  m_bStatesDirty = true;
}


const WTagSet& WReflectionProbeComponentBase::GetExcludeTags() const
{
  return m_Desc.m_ExcludeTags;
}

void WReflectionProbeComponentBase::InsertExcludeTag(const char* szTag)
{
  m_Desc.m_ExcludeTags.SetByName(szTag);
  m_bStatesDirty = true;
}

void WReflectionProbeComponentBase::RemoveExcludeTag(const char* szTag)
{
  m_Desc.m_ExcludeTags.RemoveByName(szTag);
  m_bStatesDirty = true;
}

void WReflectionProbeComponentBase::SetNearPlane(float fNearPlane)
{
  m_Desc.m_fNearPlane = fNearPlane;
  m_bStatesDirty = true;
}

void WReflectionProbeComponentBase::SetFarPlane(float fFarPlane)
{
  m_Desc.m_fFarPlane = fFarPlane;
  m_bStatesDirty = true;
}

void WReflectionProbeComponentBase::SetCaptureOffset(const WVec3& vOffset)
{
  m_Desc.m_vCaptureOffset = vOffset;
  m_bStatesDirty = true;
}

void WReflectionProbeComponentBase::SetShowDebugInfo(bool bShowDebugInfo)
{
  m_Desc.m_bShowDebugInfo = bShowDebugInfo;
  m_bStatesDirty = true;
}

bool WReflectionProbeComponentBase::GetShowDebugInfo() const
{
  return m_Desc.m_bShowDebugInfo;
}

void WReflectionProbeComponentBase::SetShowMipMaps(bool bShowMipMaps)
{
  m_Desc.m_bShowMipMaps = bShowMipMaps;
  m_bStatesDirty = true;
}

bool WReflectionProbeComponentBase::GetShowMipMaps() const
{
  return m_Desc.m_bShowMipMaps;
}

void WReflectionProbeComponentBase::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  WStreamWriter& s = inout_stream.GetStream();

  m_Desc.m_IncludeTags.Save(s);
  m_Desc.m_ExcludeTags.Save(s);
  s << m_Desc.m_Mode;
  s << m_Desc.m_bShowDebugInfo;
  s << m_Desc.m_uniqueID;
  s << m_Desc.m_fNearPlane;
  s << m_Desc.m_fFarPlane;
  s << m_Desc.m_vCaptureOffset;
}

void WReflectionProbeComponentBase::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const WUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  WStreamReader& s = inout_stream.GetStream();

  m_Desc.m_IncludeTags.Load(s, WTagRegistry::GetGlobalRegistry());
  m_Desc.m_ExcludeTags.Load(s, WTagRegistry::GetGlobalRegistry());
  s >> m_Desc.m_Mode;
  s >> m_Desc.m_bShowDebugInfo;
  s >> m_Desc.m_uniqueID;
  s >> m_Desc.m_fNearPlane;
  s >> m_Desc.m_fFarPlane;
  s >> m_Desc.m_vCaptureOffset;
}

float WReflectionProbeComponentBase::ComputePriority(WMsgExtractRenderData& msg, WReflectionProbeRenderData* pRenderData, float fVolume, const WVec3& vScale) const
{
  float fPriority = 0.0f;
  const float fLogVolume = WMath::Log2(1.0f + fVolume); // +1 to make sure it never goes negative.
  // This sorting is only by size to make sure the probes in a cluster are iterating from smallest to largest on the GPU. Which probes are actually used is determined below by the returned priority.
  pRenderData->m_uiSortingKey = WMath::FloatToInt32(static_cast<float>(WMath::MaxValue<WUInt32>()) * fLogVolume / 40.0f);

  // #TODO This is a pretty poor distance / size based score.
  if (msg.m_pView)
  {
    if (auto pCamera = msg.m_pView->GetLodCamera())
    {
      float fDistance = (pCamera->GetPosition() - pRenderData->m_vGlobalPosition).GetLength();
      float fRadius = (WMath::Abs(vScale.x) + WMath::Abs(vScale.y) + WMath::Abs(vScale.z)) / 3.0f;
      fPriority = fRadius / fDistance;
    }
  }

#ifdef W_SHOW_REFLECTION_PROBE_PRIORITIES
  WStringBuilder s;
  s.SetFormat("{}, {}", pRenderData->m_uiSortingKey, fPriority);
  WDebugRenderer::Draw3DText(GetWorld(), s, pRenderData->m_GlobalTransform.m_vPosition, WColor::Wheat);
#endif
  return fPriority;
}

W_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_ReflectionProbeComponentBase);
