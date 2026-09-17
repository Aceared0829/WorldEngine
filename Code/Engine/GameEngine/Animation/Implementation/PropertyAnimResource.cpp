#include <GameEngine/GameEnginePCH.h>

#include <Core/Curves/ColorGradientResource.h>
#include <Core/Curves/Curve1DResource.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <GameEngine/Animation/PropertyAnimResource.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WPropertyAnimResource, 1, WRTTIDefaultAllocator<WPropertyAnimResource>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_ENUM(WPropertyAnimTarget, 1)
W_ENUM_CONSTANTS(WPropertyAnimTarget::Number, WPropertyAnimTarget::VectorX, WPropertyAnimTarget::VectorY, WPropertyAnimTarget::VectorZ, WPropertyAnimTarget::VectorW)
W_ENUM_CONSTANTS(WPropertyAnimTarget::RotationX, WPropertyAnimTarget::RotationY, WPropertyAnimTarget::RotationZ, WPropertyAnimTarget::Color)
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_STATIC_REFLECTED_ENUM(WPropertyAnimMode, 1)
W_ENUM_CONSTANTS(WPropertyAnimMode::Once, WPropertyAnimMode::Loop, WPropertyAnimMode::BackAndForth)
W_END_STATIC_REFLECTED_ENUM;

W_RESOURCE_IMPLEMENT_COMMON_CODE(WPropertyAnimResource);
// clang-format on

WPropertyAnimResource::WPropertyAnimResource()
  : WResource(DoUpdate::OnAnyThread, 1)
{
}

W_RESOURCE_IMPLEMENT_CREATEABLE(WPropertyAnimResource, WPropertyAnimResourceDescriptor)
{
  m_pDescriptor = W_DEFAULT_NEW(WPropertyAnimResourceDescriptor);
  *m_pDescriptor = descriptor;

  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = WResourceState::Loaded;

  return res;
}

WResourceLoadDesc WPropertyAnimResource::UnloadData(Unload WhatToUnload)
{
  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = WResourceState::Unloaded;

  m_pDescriptor = nullptr;

  return res;
}

WResourceLoadDesc WPropertyAnimResource::UpdateContent(WStreamReader* Stream)
{
  W_LOG_BLOCK("WPropertyAnimResource::UpdateContent", GetResourceIdOrDescription());

  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  if (Stream == nullptr)
  {
    res.m_State = WResourceState::LoadedResourceMissing;
    return res;
  }

  // the standard file reader writes the absolute file path into the stream
  WStringBuilder sAbsFilePath;
  (*Stream) >> sAbsFilePath;

  // skip the asset file header at the start of the file
  WAssetFileHeader AssetHash;
  AssetHash.Read(*Stream).IgnoreResult();

  m_pDescriptor = W_DEFAULT_NEW(WPropertyAnimResourceDescriptor);
  m_pDescriptor->Load(*Stream);

  res.m_State = WResourceState::Loaded;
  return res;
}

void WPropertyAnimResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
  out_NewMemoryUsage.m_uiMemoryCPU = 0;

  if (m_pDescriptor)
  {
    out_NewMemoryUsage.m_uiMemoryCPU = m_pDescriptor->m_FloatAnimations.GetHeapMemoryUsage() + sizeof(WPropertyAnimResourceDescriptor);
  }
}

void WPropertyAnimResourceDescriptor::Save(WStreamWriter& inout_stream) const
{
  const WUInt8 uiVersion = 6;
  const WUInt8 uiIdentifier = 0x0A; // dummy to fill the header to 32 Bit
  const WUInt16 uiNumFloatAnimations = static_cast<WUInt16>(m_FloatAnimations.GetCount());
  const WUInt16 uiNumColorAnimations = static_cast<WUInt16>(m_ColorAnimations.GetCount());

  W_ASSERT_DEV(m_AnimationDuration.GetSeconds() > 0, "Animation duration must be positive");

  inout_stream << uiVersion;
  inout_stream << uiIdentifier;
  inout_stream << m_AnimationDuration;
  inout_stream << uiNumFloatAnimations;

  WCurve1D tmpCurve;

  for (WUInt32 i = 0; i < uiNumFloatAnimations; ++i)
  {
    inout_stream << m_FloatAnimations[i].m_sObjectSearchSequence;
    inout_stream << m_FloatAnimations[i].m_sComponentType;
    inout_stream << m_FloatAnimations[i].m_sPropertyPath;
    inout_stream << m_FloatAnimations[i].m_Target;

    tmpCurve = m_FloatAnimations[i].m_Curve;
    tmpCurve.SortControlPoints();
    tmpCurve.ApplyTangentModes();
    tmpCurve.ClampTangents();
    tmpCurve.Save(inout_stream);
  }

  WColorGradient tmpGradient;
  inout_stream << uiNumColorAnimations;
  for (WUInt32 i = 0; i < uiNumColorAnimations; ++i)
  {
    inout_stream << m_ColorAnimations[i].m_sObjectSearchSequence;
    inout_stream << m_ColorAnimations[i].m_sComponentType;
    inout_stream << m_ColorAnimations[i].m_sPropertyPath;
    inout_stream << m_ColorAnimations[i].m_Target;

    tmpGradient = m_ColorAnimations[i].m_Gradient;
    tmpGradient.Save(inout_stream);
  }

  // Version 6
  m_EventTrack.Save(inout_stream);
}

void WPropertyAnimResourceDescriptor::Load(WStreamReader& inout_stream)
{
  WUInt8 uiVersion = 0;
  WUInt8 uiIdentifier = 0;
  WUInt16 uiNumAnimations = 0;

  inout_stream >> uiVersion;
  inout_stream >> uiIdentifier;

  W_ASSERT_DEV(uiIdentifier == 0x0A, "File does not contain a valid WPropertyAnimResourceDescriptor");
  W_ASSERT_DEV(uiVersion == 4 || uiVersion == 5 || uiVersion == 6, "Invalid file version {0}", uiVersion);

  inout_stream >> m_AnimationDuration;

  if (uiVersion == 4)
  {
    WEnum<WPropertyAnimMode> mode;
    inout_stream >> mode;
  }

  inout_stream >> uiNumAnimations;
  m_FloatAnimations.SetCount(uiNumAnimations);

  for (WUInt32 i = 0; i < uiNumAnimations; ++i)
  {
    auto& anim = m_FloatAnimations[i];

    inout_stream >> anim.m_sObjectSearchSequence;
    inout_stream >> anim.m_sComponentType;
    inout_stream >> anim.m_sPropertyPath;
    inout_stream >> anim.m_Target;
    anim.m_Curve.Load(inout_stream);
    anim.m_Curve.SortControlPoints();
    anim.m_Curve.CreateLinearApproximation();

    if (!anim.m_sComponentType.IsEmpty())
      anim.m_pComponentRtti = WRTTI::FindTypeByName(anim.m_sComponentType);
  }

  inout_stream >> uiNumAnimations;
  m_ColorAnimations.SetCount(uiNumAnimations);

  for (WUInt32 i = 0; i < uiNumAnimations; ++i)
  {
    auto& anim = m_ColorAnimations[i];

    inout_stream >> anim.m_sObjectSearchSequence;
    inout_stream >> anim.m_sComponentType;
    inout_stream >> anim.m_sPropertyPath;
    inout_stream >> anim.m_Target;
    anim.m_Gradient.Load(inout_stream);

    if (!anim.m_sComponentType.IsEmpty())
      anim.m_pComponentRtti = WRTTI::FindTypeByName(anim.m_sComponentType);
  }

  if (uiVersion >= 6)
  {
    m_EventTrack.Load(inout_stream);
  }
}



W_STATICLINK_FILE(GameEngine, GameEngine_Animation_Implementation_PropertyAnimResource);
