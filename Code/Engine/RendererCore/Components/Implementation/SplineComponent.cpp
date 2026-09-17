#include <GameEngine/GameEnginePCH.h>

#include <Core/Messages/HierarchyChangedMessages.h>
#include <Core/Messages/TransformChangedMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <RendererCore/Components/SplineComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_BITFLAGS(WSplineComponentFlags, 1)
  W_BITFLAGS_CONSTANTS(WSplineComponentFlags::VisualizeSpline, WSplineComponentFlags::VisualizeUpDir, WSplineComponentFlags::VisualizeTangents)
W_END_STATIC_REFLECTED_BITFLAGS;

W_BEGIN_STATIC_REFLECTED_ENUM(WSplineComponentSpace, 1)
  W_ENUM_CONSTANTS(WSplineComponentSpace::Local, WSplineComponentSpace::Global)
W_END_STATIC_REFLECTED_ENUM;

W_IMPLEMENT_MESSAGE_TYPE(WMsgSplineChanged);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMsgSplineChanged, 1, WRTTIDefaultAllocator<WMsgSplineChanged>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("ChangeCounter", m_uiChangeCounter),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

WSplineComponentManager::WSplineComponentManager(WWorld* pWorld)
  : WComponentManager(pWorld)
{
}

void WSplineComponentManager::SetEnableUpdate(WSplineComponent* pThis, bool bEnable)
{
  if (bEnable)
  {
    if (!m_NeedUpdate.Contains(pThis))
      m_NeedUpdate.PushBack(pThis);
  }
  else
  {
    m_NeedUpdate.RemoveAndSwap(pThis);
  }
}

void WSplineComponentManager::Initialize()
{
  auto desc = W_CREATE_MODULE_UPDATE_FUNCTION_DESC(WSplineComponentManager::Update, this);
  desc.m_bOnlyUpdateWhenSimulating = false;
  desc.m_Phase = WWorldUpdatePhase::PostTransform;

  this->RegisterUpdateFunction(desc);
}

void WSplineComponentManager::Update(const WWorldModule::UpdateContext& context)
{
  for (WSplineComponent* pComponent : m_NeedUpdate)
  {
    if (pComponent->IsActiveAndInitialized())
    {
      pComponent->DrawDebugVisualizations(pComponent->GetSplineFlags());
    }
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_COMPONENT_TYPE(WSplineComponent, 2, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_BITFLAGS_ACCESSOR_PROPERTY("Flags", WSplineComponentFlags, GetSplineFlags, SetSplineFlags),
    W_ACCESSOR_PROPERTY("Closed", GetClosed, SetClosed),
    W_MEMBER_PROPERTY("EditNodes", m_bEditDummy),
  }
  W_END_PROPERTIES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgSplineChanged, OnMsgSplineChanged),
    W_MESSAGE_HANDLER(WMsgExtractRenderData, OnMsgExtractRenderData),
  }
  W_END_MESSAGEHANDLERS;
  W_BEGIN_FUNCTIONS
  {
    W_SCRIPT_FUNCTION_PROPERTY(GetPositionAtKey, In, "Key", In, "Space"),
    W_SCRIPT_FUNCTION_PROPERTY(GetForwardDirAtKey, In, "Key", In, "Space"),
    W_SCRIPT_FUNCTION_PROPERTY(GetUpDirAtKey, In, "Key", In, "Space"),
    W_SCRIPT_FUNCTION_PROPERTY(GetScaleAtKey, In, "Key", In, "Space"),
    W_SCRIPT_FUNCTION_PROPERTY(GetTransformAtKey, In, "Key", In, "Space"),

    W_SCRIPT_FUNCTION_PROPERTY(GetTotalLength),
    W_SCRIPT_FUNCTION_PROPERTY(GetKeyAtDistance, In, "Distance"),
    W_SCRIPT_FUNCTION_PROPERTY(GetPositionAtDistance, In, "Distance", In, "Space"),
    W_SCRIPT_FUNCTION_PROPERTY(GetForwardDirAtDistance, In, "Distance", In, "Space"),
    W_SCRIPT_FUNCTION_PROPERTY(GetUpDirAtDistance, In, "Distance", In, "Space"),
    W_SCRIPT_FUNCTION_PROPERTY(GetScaleAtDistance, In, "Distance", In, "Space"),
    W_SCRIPT_FUNCTION_PROPERTY(GetTransformAtDistance, In, "Distance", In, "Space"),

    W_SCRIPT_FUNCTION_PROPERTY(FindKeyClosestToPoint, In, "Point", Out, "DistanceToPoint", In, "Space", In, "MaxError")->AddAttributes(new WFunctionArgumentAttributes(3, new WDefaultValueAttribute(0.1))),

    W_SCRIPT_FUNCTION_PROPERTY(GetChangeCounter),

    W_FUNCTION_PROPERTY(OnObjectCreated),
    W_FUNCTION_PROPERTY(SetChildOrder),
  }
  W_END_FUNCTIONS;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Utilities/Splines"),
    new WSyncChildOrderAttribute(),
    new WSplineManipulatorAttribute("EditNodes", "Closed"),
  }
  W_END_ATTRIBUTES;
}
W_END_COMPONENT_TYPE
// clang-format on

enum SplineComponentInternalFlags
{
  DisallowUpdateFromNodes = 0,
};

WSplineComponent::WSplineComponent() = default;
WSplineComponent::~WSplineComponent() = default;

void WSplineComponent::SerializeComponent(WWorldWriter& ref_stream) const
{
  SUPER::SerializeComponent(ref_stream);

  auto& s = ref_stream.GetStream();
  s << m_SplineFlags;
  m_Spline.Serialize(s).AssertSuccess();

  s << m_Uuid;
}

void WSplineComponent::DeserializeComponent(WWorldReader& ref_stream)
{
  SUPER::DeserializeComponent(ref_stream);
  WUInt32 uiVersion = ref_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = ref_stream.GetStream();
  s >> m_SplineFlags;
  m_Spline.Deserialize(s).AssertSuccess();

  if (uiVersion >= 2)
  {
    s >> m_Uuid;
  }

  // This is to prevent the spline from getting cleared in the Editor when it is deserialized as part of a prefab.
  // In this case it still has a unique id so the 'in editor' check in UpdateSpline alone would not be sufficient.
  SetUserFlag(SplineComponentInternalFlags::DisallowUpdateFromNodes, true);
}

void WSplineComponent::OnActivated()
{
  SUPER::OnActivated();

  if (m_SplineFlags.IsAnyFlagSet())
  {
    static_cast<WSplineComponentManager*>(GetOwningManager())->SetEnableUpdate(this, true);
  }

  UpdateSpline();
}

void WSplineComponent::OnDeactivated()
{
  SUPER::OnDeactivated();

  if (m_SplineFlags.IsAnyFlagSet())
  {
    // assume that if no flag is set, update is already disabled
    static_cast<WSplineComponentManager*>(GetOwningManager())->SetEnableUpdate(this, false);
  }
}

void WSplineComponent::SetClosed(bool bClosed)
{
  if (m_Spline.m_bClosed == bClosed)
    return;

  m_Spline.m_bClosed = bClosed;

  UpdateSpline();
}

void WSplineComponent::SetSplineFlags(WBitflags<WSplineComponentFlags> flags)
{
  if (m_SplineFlags == flags)
    return;

  m_SplineFlags = flags;

  if (IsActiveAndInitialized())
  {
    static_cast<WSplineComponentManager*>(GetOwningManager())->SetEnableUpdate(this, m_SplineFlags.IsAnyFlagSet());
  }
}

WVec3 WSplineComponent::GetPositionAtKey(float fKey, WEnum<WSplineComponentSpace> space /* = WSplineComponentSpace::Default*/) const
{
  WSimdVec4f pos = m_Spline.EvaluatePosition(fKey);

  if (space == WSplineComponentSpace::Global)
  {
    pos = GetOwner()->GetGlobalTransformSimd().TransformPosition(pos);
  }

  return WSimdConversion::ToVec3(pos);
}

WVec3 WSplineComponent::GetForwardDirAtKey(float fKey, WEnum<WSplineComponentSpace> space /* = WSplineComponentSpace::Default*/) const
{
  WSimdVec4f dir = m_Spline.EvaluateDerivative(fKey);
  dir.NormalizeIfNotZero<3>(WSimdVec4f(1, 0, 0, 0), 0.0001f);

  if (space == WSplineComponentSpace::Global)
  {
    dir = GetOwner()->GetGlobalTransformSimd().TransformDirection(dir);
  }

  return WSimdConversion::ToVec3(dir);
}

WVec3 WSplineComponent::GetUpDirAtKey(float fKey, WEnum<WSplineComponentSpace> space /* = WSplineComponentSpace::Default*/) const
{
  WSimdVec4f dir = m_Spline.EvaluateUpDirection(fKey);

  if (space == WSplineComponentSpace::Global)
  {
    dir = GetOwner()->GetGlobalTransformSimd().TransformDirection(dir);
  }

  return WSimdConversion::ToVec3(dir);
}

WVec3 WSplineComponent::GetScaleAtKey(float fKey, WEnum<WSplineComponentSpace> space /* = WSplineComponentSpace::Default*/) const
{
  WSimdVec4f scale = m_Spline.EvaluateScale(fKey);

  if (space == WSplineComponentSpace::Global)
  {
    scale = scale.CompMul(GetOwner()->GetGlobalTransformSimd().m_Scale);
  }

  return WSimdConversion::ToVec3(scale);
}

WTransform WSplineComponent::GetTransformAtKey(float fKey, WEnum<WSplineComponentSpace> space /* = WSplineComponentSpace::Default*/) const
{
  WSimdTransform t = m_Spline.EvaluateTransform(fKey);

  if (space == WSplineComponentSpace::Global)
  {
    t = WSimdTransform::MakeGlobalTransform(GetOwner()->GetGlobalTransformSimd(), t);
  }

  return WSimdConversion::ToTransform(t);
}

float WSplineComponent::GetSegmentLength(WUInt32 uiSegmentIndex) const
{
  const float fSegmentKey = static_cast<float>(uiSegmentIndex);
  const float fNextSegmentKey = static_cast<float>(uiSegmentIndex + 1);

  float fStartDistance = 0.0f;
  float fEndDistance = 0.0f;
  for (auto it : m_DistanceToKey)
  {
    if (WMath::IsEqual(it.value, fSegmentKey, WMath::DefaultEpsilon<float>()))
    {
      fStartDistance = it.key;
    }
    if (WMath::IsEqual(it.value, fNextSegmentKey, WMath::DefaultEpsilon<float>()))
    {
      fEndDistance = it.key;
      break;
    }
  }

  return fEndDistance - fStartDistance;
}

WVec3 WSplineComponent::GetPositionAtDistance(float fDistance, WEnum<WSplineComponentSpace> space /* = WSplineComponentSpace::Default*/) const
{
  const float fKey = GetKeyAtDistance(fDistance);
  return GetPositionAtKey(fKey, space);
}

WVec3 WSplineComponent::GetForwardDirAtDistance(float fDistance, WEnum<WSplineComponentSpace> space /* = WSplineComponentSpace::Default*/) const
{
  const float fKey = GetKeyAtDistance(fDistance);
  return GetForwardDirAtKey(fKey, space);
}

WVec3 WSplineComponent::GetUpDirAtDistance(float fDistance, WEnum<WSplineComponentSpace> space /* = WSplineComponentSpace::Default*/) const
{
  const float fKey = GetKeyAtDistance(fDistance);
  return GetUpDirAtKey(fKey, space);
}

WVec3 WSplineComponent::GetScaleAtDistance(float fDistance, WEnum<WSplineComponentSpace> space /* = WSplineComponentSpace::Default*/) const
{
  const float fKey = GetKeyAtDistance(fDistance);
  return GetScaleAtKey(fKey, space);
}

WTransform WSplineComponent::GetTransformAtDistance(float fDistance, WEnum<WSplineComponentSpace> space /* = WSplineComponentSpace::Default*/) const
{
  const float fKey = GetKeyAtDistance(fDistance);
  return GetTransformAtKey(fKey, space);
}

float WSplineComponent::FindKeyClosestToPoint(const WVec3& vPoint, float& out_fDistance, WEnum<WSplineComponentSpace> space /* = WSplineComponentSpace::Default*/, float fMaxError /*= 0.1f*/) const
{
  WSimdVec4f p = WSimdConversion::ToVec3(vPoint);

  if (space == WSplineComponentSpace::Global)
  {
    p = GetOwner()->GetGlobalTransformSimd().GetInverse().TransformPosition(p);
  }

  float fClosestKey = 0.0f;
  float fClosestDistSqr = 0.0f;
  m_Spline.FindClosestPoint(p, fClosestKey, fClosestDistSqr, fMaxError);

  out_fDistance = WMath::Sqrt(fClosestDistSqr);
  return fClosestKey;
}

void WSplineComponent::SetSpline(WSpline&& spline)
{
  WUInt32 uiOldChangeCounter = m_Spline.m_uiChangeCounter;
  m_Spline = std::move(spline);
  m_Spline.m_uiChangeCounter = uiOldChangeCounter + 1;

  CreateDistanceToKeyRemapping();

  SendSplineChangedEvent();
}

void WSplineComponent::EndModifySpline(bool bRecreateDistanceToKeyMapping /*= true*/)
{
  m_Spline.m_uiChangeCounter++;

  if (bRecreateDistanceToKeyMapping || m_DistanceToKey.IsEmpty())
  {
    CreateDistanceToKeyRemapping();
  }

  SendSplineChangedEvent();
}

float WSplineComponent::GetKeyAtDistanceHelper(const WArrayMap<float, float>& distanceToKey, float fDistance)
{
  if (distanceToKey.IsEmpty())
    return 0.0f;

  const WUInt32 uiUpperIndex = WMath::Min(distanceToKey.UpperBound(fDistance), distanceToKey.GetCount() - 1);
  const WUInt32 uiLowerIndex = uiUpperIndex > 0 ? uiUpperIndex - 1 : 0;

  const float fLowerDistance = distanceToKey.GetKey(uiLowerIndex);
  const float fUpperDistance = distanceToKey.GetKey(uiUpperIndex);
  const float fLowerKey = distanceToKey.GetValue(uiLowerIndex);
  const float fUpperKey = distanceToKey.GetValue(uiUpperIndex);

  return WMath::Lerp(fLowerKey, fUpperKey, WMath::Saturate(WMath::Unlerp(fLowerDistance, fUpperDistance, fDistance)));
}

void WSplineComponent::OnMsgSplineChanged(WMsgSplineChanged& ref_msg)
{
  UpdateSpline();
}

void WSplineComponent::SetChildOrder(const WVariantArray& handles)
{
  m_Nodes.Clear();
  m_Nodes.Reserve(handles.GetCount());

  for (const WVariant& v : handles)
    m_Nodes.PushBack(v.Get<WGameObjectHandle>());

  UpdateSpline();
}

void WSplineComponent::SendSplineChangedEvent()
{
  WMsgSplineChanged msg;
  msg.m_uiChangeCounter = m_Spline.m_uiChangeCounter;

  for (WComponent* pComp : GetOwner()->GetComponents())
  {
    if (pComp != this)
    {
      pComp->SendMessage(msg);
    }
  }
}

void WSplineComponent::UpdateSpline(bool bSendChangedEvent /* = true*/)
{
  if (!IsActiveAndInitialized())
    return;

  if (GetUniqueID() != WInvalidIndex && !GetUserFlag(SplineComponentInternalFlags::DisallowUpdateFromNodes))
  {
    // Only in Editor
    UpdateFromNodeObjects();
  }

  CreateDistanceToKeyRemapping();

  if (bSendChangedEvent)
  {
    SendSplineChangedEvent();
  }
}

void WSplineComponent::UpdateFromNodeObjects()
{
  W_ASSERT_DEV(!GetUserFlag(SplineComponentInternalFlags::DisallowUpdateFromNodes), "This function should not be called when updates from nodes are disabled.");

  auto& points = m_Spline.m_ControlPoints;
  points.Clear();

  if (m_Nodes.GetCount() < 2)
    return;

  for (WGameObjectHandle hNode : m_Nodes)
  {
    WGameObject* pNode;
    if (!GetWorld()->TryGetObject(hNode, pNode))
      continue;

    WSplineNodeComponent* pNodeComponent = nullptr;
    if (!pNode->TryGetComponentOfBaseType(pNodeComponent))
      continue;

    pNodeComponent->m_uiNodeIndex = points.GetCount();

    const WSimdTransform localNodeTransform = pNodeComponent->GetOwner()->GetLocalTransformSimd();

    auto& cp = points.ExpandAndGetRef();
    cp.SetPosition(localNodeTransform.m_Position);
    cp.SetTangentIn(pNodeComponent->GetFinalCustomTangentIn(), pNodeComponent->GetTangentModeIn());

    if (pNodeComponent->GetLinkCustomTangents() && pNodeComponent->GetTangentModeIn() == WSplineTangentMode::Custom && pNodeComponent->GetTangentModeOut() == WSplineTangentMode::Custom)
    {
      cp.SetTangentOut(-pNodeComponent->GetFinalCustomTangentIn(), pNodeComponent->GetTangentModeOut());
    }
    else
    {
      cp.SetTangentOut(pNodeComponent->GetFinalCustomTangentOut(), pNodeComponent->GetTangentModeOut());
    }

    cp.SetRoll(pNodeComponent->GetRoll());
    cp.SetScale(localNodeTransform.m_Scale);
  }

  if (points.GetCount() < 2)
  {
    points.Clear();
    return;
  }

  WCoordinateSystem coordinateSystem;
  GetWorld()->GetCoordinateSystem(GetOwner()->GetGlobalPosition(), coordinateSystem);

  m_Spline.CalculateUpDirAndAutoTangents(WSimdConversion::ToVec3(coordinateSystem.m_vUpDir), WSimdConversion::ToVec3(coordinateSystem.m_vForwardDir));

  ++m_Spline.m_uiChangeCounter;
  if (m_Spline.m_uiChangeCounter == WInvalidIndex)
    m_Spline.m_uiChangeCounter = 0;
}

void WSplineComponent::InsertHalfPoint(WDynamicArray<float>& ref_Ts, WUInt32 uiCp0, float fLowerT, float fUpperT, const WSimdVec4f& vLowerPos, const WSimdVec4f& vUpperPos, float fDistSqr, WInt32 iMinSteps, WInt32 iMaxSteps) const
{
  const float fHalfT = WMath::Lerp(fLowerT, fUpperT, 0.5f);

  const WSimdVec4f vHalfPos = m_Spline.EvaluatePosition(uiCp0, fHalfT);

  if (iMinSteps <= 0)
  {
    const WSimdVec4f vInterpPos = WSimdVec4f::Lerp(vLowerPos, vUpperPos, WSimdVec4f(0.5f));
    if ((vHalfPos - vInterpPos).GetLengthSquared<3>() < fDistSqr)
    {
      return;
    }
  }

  if (iMaxSteps > 0)
  {
    InsertHalfPoint(ref_Ts, uiCp0, fLowerT, fHalfT, vLowerPos, vHalfPos, fDistSqr, iMinSteps - 1, iMaxSteps - 1);
  }

  ref_Ts.PushBack(fHalfT);

  if (iMaxSteps > 0)
  {
    InsertHalfPoint(ref_Ts, uiCp0, fHalfT, fUpperT, vHalfPos, vUpperPos, fDistSqr, iMinSteps - 1, iMaxSteps - 1);
  }
}

void WSplineComponent::CreateDistanceToKeyRemapping()
{
  m_DistanceToKey.Clear();
  m_fTotalLength = 0.0f;

  const auto& points = m_Spline.m_ControlPoints;
  const WUInt32 uiNumCPs = points.GetCount();
  if (uiNumCPs < 2)
    return;

  m_DistanceToKey.Insert(0.0f, 0.0f);
  constexpr float fMaxErrorSqr = WMath::Square(0.1f);

  WTempHybridArray<float, 64> segmentTs;
  const WUInt32 uiNumSegments = m_Spline.m_bClosed ? uiNumCPs : uiNumCPs - 1;
  for (WUInt32 uiSegment = 0; uiSegment < uiNumSegments; ++uiSegment)
  {
    segmentTs.Clear();
    segmentTs.PushBack(1.0f);

    const WUInt32 uiCp0 = uiSegment;
    const WUInt32 uiCp1 = (uiCp0 + 1 < uiNumCPs) ? uiCp0 + 1 : 0;
    const auto& vLowerPos = points[uiCp0].m_vPos;
    const auto& vUpperPos = points[uiCp1].m_vPos;
    W_ASSERT_DEBUG(vLowerPos.IsValid<3>() && vUpperPos.IsValid<3>(), "Invalid control point position.");

    InsertHalfPoint(segmentTs, uiCp0, 0.0f, 1.0f, vLowerPos, vUpperPos, fMaxErrorSqr, 0, 7);
    segmentTs.Sort();

    WSimdVec4f vLastPos = vLowerPos;
    for (float t : segmentTs)
    {
      const WSimdVec4f vCurPos = m_Spline.EvaluatePosition(uiCp0, t);
      m_fTotalLength += (vLastPos - vCurPos).GetLength<3>();

      m_DistanceToKey.Insert(m_fTotalLength, t + uiSegment);

      vLastPos = vCurPos;
    }
  }
}

void WSplineComponent::DrawDebugVisualizations(WBitflags<WSplineComponentFlags> flags) const
{
  if (flags.IsNoFlagSet())
    return;

  if (m_DistanceToKey.IsEmpty())
    return;

  const bool bVisPath = flags.IsSet(WSplineComponentFlags::VisualizeSpline);
  const bool bVisUp = flags.IsSet(WSplineComponentFlags::VisualizeUpDir);

  WTempHybridArray<WDebugRendererLine, 32> lines;
  WColor c = WColorScheme::LightUI(WColorScheme::Pink);
  WColor cUp = WColorScheme::LightUI(WColorScheme::Blue);

  WVec3 lastPos = GetPositionAtKey(0);
  float fLastKey = 0.0f;
  for (WUInt32 i = 1; i < m_DistanceToKey.GetCount(); ++i)
  {
    const float fKey = m_DistanceToKey.GetValue(i);

    const float fSubStep = 1.0f / 4.0f;
    for (float fT = fSubStep; fT <= 1.01f; fT += fSubStep)
    {
      const float fSubKey = WMath::Lerp(fLastKey, fKey, fT);
      const WVec3 curPos = GetPositionAtKey(fSubKey);

      if (bVisPath)
      {
        auto& line = lines.ExpandAndGetRef();
        line.m_start = lastPos;
        line.m_end = curPos;
        line.m_startColor = c;
        line.m_endColor = c;
      }

      if (bVisUp)
      {
        auto& line = lines.ExpandAndGetRef();
        line.m_start = curPos;
        line.m_end = curPos + GetUpDirAtKey(fSubKey) * 0.25f;
        line.m_startColor = cUp;
        line.m_endColor = cUp;
      }

      lastPos = curPos;
    }

    fLastKey = fKey;
  }

  WDebugRenderer::DrawLinesOccluded(GetWorld(), lines, WColor::White.GetDarker(), GetOwner()->GetGlobalTransform());
  WDebugRenderer::DrawLines(GetWorld(), lines, WColor::White, GetOwner()->GetGlobalTransform());

  const bool bVisTangents = flags.IsSet(WSplineComponentFlags::VisualizeTangents);
  if (bVisTangents)
  {
    for (WUInt32 i = 0; i < m_Spline.m_ControlPoints.GetCount(); ++i)
    {
      DrawDebugTangents(i);
    }
  }
}

void WSplineComponent::DrawDebugTangents(WUInt32 uiPointIndex, WSplineTangentMode::Enum tangentModeIn /*= WSplineTangentMode::Default*/, WSplineTangentMode::Enum tangentModeOut /*= WSplineTangentMode::Default*/) const
{
  if (uiPointIndex < m_Spline.m_ControlPoints.GetCount())
  {
    const WSpline::ControlPoint& cp = m_Spline.m_ControlPoints[uiPointIndex];

    const WColor tInColor = WColorScheme::DarkUI(tangentModeIn == WSplineTangentMode::Custom ? WColorScheme::Grape : WColorScheme::Gray);
    const WColor tOutColor = WColorScheme::DarkUI(tangentModeOut == WSplineTangentMode::Custom ? WColorScheme::Grape : WColorScheme::Gray);
    const WVec3 vTangentIn = WSimdConversion::ToVec3(cp.m_vPosTangentIn);
    const WVec3 vTangentOut = WSimdConversion::ToVec3(cp.m_vPosTangentOut);

    const WVec3 vGlobalPos = WSimdConversion::ToVec3(GetOwner()->GetGlobalTransformSimd().TransformPosition(cp.m_vPos));
    const WTransform t = WTransform::Make(vGlobalPos, GetOwner()->GetGlobalRotation(), GetOwner()->GetGlobalScaling());

    WDebugRenderer::DrawLineSphere(GetWorld(), WBoundingSphere::MakeFromCenterAndRadius(vTangentIn, 0.05f), tInColor, t);
    WDebugRenderer::DrawLineSphere(GetWorld(), WBoundingSphere::MakeFromCenterAndRadius(vTangentOut, 0.05f), tOutColor, t);

    WTempHybridArray<WDebugRendererLine, 2> lines;
    lines.PushBack(WDebugRendererLine(WVec3::MakeZero(), vTangentIn, tInColor));
    lines.PushBack(WDebugRendererLine(WVec3::MakeZero(), vTangentOut, tOutColor));
    WDebugRenderer::DrawLines(GetWorld(), lines, WColor::White, t);
  }
}

bool WSplineComponent::DrawSplineOnSelection() const
{
  const WUInt16 uiFrame = static_cast<WUInt16>(WRenderWorld::GetFrameCounter());
  if (m_uiExtractedFrame == uiFrame)
  {
    return false; // already drawn this frame
  }

  m_uiExtractedFrame = uiFrame;

  if (!m_SplineFlags.IsSet(WSplineComponentFlags::VisualizeSpline))
  {
    DrawDebugVisualizations(WSplineComponentFlags::VisualizeSpline);
  }

  return true;
}

void WSplineComponent::OnMsgExtractRenderData(WMsgExtractRenderData& msg) const
{
  if (msg.m_OverrideCategory != WDefaultRenderDataCategories::Selection)
    return;

  DrawSplineOnSelection();
}

void WSplineComponent::OnObjectCreated(const WAbstractObjectNode& node)
{
  m_Uuid = node.GetGuid();
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_COMPONENT_TYPE(WSplineNodeComponent, 1, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("Roll", GetRoll, SetRoll),
    W_ENUM_ACCESSOR_PROPERTY("TangentModeIn", WSplineTangentMode, GetTangentModeIn, SetTangentModeIn),
    W_ACCESSOR_PROPERTY("CustomTangentIn", GetCustomTangentIn, SetCustomTangentIn),
    W_ENUM_ACCESSOR_PROPERTY("TangentModeOut", WSplineTangentMode, GetTangentModeOut, SetTangentModeOut),
    W_ACCESSOR_PROPERTY("CustomTangentOut", GetCustomTangentOut, SetCustomTangentOut),
    W_ACCESSOR_PROPERTY("LinkCustomTangents", GetLinkCustomTangents, SetLinkCustomTangents),
    W_MEMBER_PROPERTY("EditNodes", m_bEditDummy),
  }
  W_END_PROPERTIES;

  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgTransformChanged, OnMsgTransformChanged),
    W_MESSAGE_HANDLER(WMsgParentChanged, OnMsgParentChanged),
    W_MESSAGE_HANDLER(WMsgExtractRenderData, OnMsgExtractRenderData),
  }
  W_END_MESSAGEHANDLERS;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Utilities/Splines"),
    new WShapeIconAlwaysVisibleAttribute(),
    new WSplineTangentManipulatorAttribute("TangentModeIn", "CustomTangentIn"),
    new WSplineTangentManipulatorAttribute("TangentModeOut", "CustomTangentOut"),
    new WSplineManipulatorAttribute("EditNodes", "Closed"),
  }
  W_END_ATTRIBUTES;
}
W_END_COMPONENT_TYPE
// clang-format on

WSplineNodeComponent::WSplineNodeComponent() = default;
WSplineNodeComponent::~WSplineNodeComponent() = default;

void WSplineNodeComponent::SetRoll(WAngle roll)
{
  if (m_Roll != roll)
  {
    m_Roll = roll;
    SplineChanged();
  }
}

void WSplineNodeComponent::SetTangentModeIn(WEnum<WSplineTangentMode> mode)
{
  if (m_TangentModeIn != mode)
  {
    m_TangentModeIn = mode;
    SplineChanged();
  }
}

void WSplineNodeComponent::SetTangentModeOut(WEnum<WSplineTangentMode> mode)
{
  if (m_TangentModeOut != mode)
  {
    m_TangentModeOut = mode;
    SplineChanged();
  }
}

void WSplineNodeComponent::SetCustomTangentIn(const WVec3& vTangent)
{
  if (m_vCustomTangentIn != vTangent)
  {
    m_vCustomTangentIn = vTangent;
    SplineChanged();
  }
}

void WSplineNodeComponent::SetCustomTangentOut(const WVec3& vTangent)
{
  if (m_vCustomTangentOut != vTangent)
  {
    m_vCustomTangentOut = vTangent;
    SplineChanged();
  }
}

void WSplineNodeComponent::SetLinkCustomTangents(bool bLink)
{
  if (m_bLinkCustomTangents != bLink)
  {
    m_bLinkCustomTangents = bLink;
    SplineChanged();
  }
}

void WSplineNodeComponent::OnMsgTransformChanged(WMsgTransformChanged& msg)
{
  SplineChanged();
}

void WSplineNodeComponent::OnMsgParentChanged(WMsgParentChanged& msg)
{
  if (msg.m_Type == WMsgParentChanged::Type::ParentUnlinked)
  {
    WGameObject* pOldParent = nullptr;
    if (GetWorld()->TryGetObject(msg.m_hParent, pOldParent))
    {
      WMsgSplineChanged msg2;
      pOldParent->SendEventMessage(msg2, this);
    }
  }
  else
  {
    SplineChanged();
  }
}

void WSplineNodeComponent::OnMsgExtractRenderData(WMsgExtractRenderData& msg) const
{
  if (msg.m_OverrideCategory != WDefaultRenderDataCategories::Selection)
    return;

  const WGameObject* pParent = GetOwner()->GetParent();
  const WSplineComponent* pSplineComponent = nullptr;

  while (pParent != nullptr && !pParent->TryGetComponentOfBaseType(pSplineComponent))
  {
    pParent = pParent->GetParent();
  }

  if (pSplineComponent != nullptr && pSplineComponent->DrawSplineOnSelection())
  {
    pSplineComponent->DrawDebugTangents(m_uiNodeIndex, m_TangentModeIn, m_TangentModeOut);
  }
}

void WSplineNodeComponent::OnActivated()
{
  SUPER::OnActivated();

  GetOwner()->EnableStaticTransformChangesNotifications();
  GetOwner()->EnableParentChangesNotifications();
}

void WSplineNodeComponent::SplineChanged()
{
  if (!IsActiveAndInitialized())
    return;

  WMsgSplineChanged msg;
  GetOwner()->SendEventMessage(msg, this);
}

WSimdVec4f WSplineNodeComponent::GetFinalCustomTangentIn() const
{
  WSimdVec4f t = WSimdConversion::ToVec3(m_vCustomTangentIn);
  t = GetOwner()->GetLocalRotationSimd() * t;
  return t;
}

WSimdVec4f WSplineNodeComponent::GetFinalCustomTangentOut() const
{
  WSimdVec4f t = WSimdConversion::ToVec3(m_vCustomTangentOut);
  t = GetOwner()->GetLocalRotationSimd() * t;
  return t;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

class WPathComponentPatch_1_2 : public WGraphPatch
{
public:
  WPathComponentPatch_1_2()
    : WGraphPatch("WPathComponent", 2)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    ref_context.RenameClass("WSplineComponent");

    auto* pFlags = pNode->FindProperty("Flags");
    if (pFlags && pFlags->m_Value.IsA<WString>())
    {
      WStringBuilder sFlags = pFlags->m_Value.Get<WString>();
      sFlags.ReplaceAll("Path", "Spline");
      pNode->ChangeProperty("Flags", sFlags.GetView());
    }
  }
};

WPathComponentPatch_1_2 g_WPathComponentPatch_1_2;

class WPathNodeComponentPatch_1_2 : public WGraphPatch
{
public:
  WPathNodeComponentPatch_1_2()
    : WGraphPatch("WPathNodeComponent", 2)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    ref_context.RenameClass("WSplineNodeComponent");
  }
};

WPathNodeComponentPatch_1_2 g_WPathNodeComponentPatch_1_2;


W_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_SplineComponent);
