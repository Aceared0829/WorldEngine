#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimAsset.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimObjectAccessor.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimObjectManager.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WPropertyAnimationTrack, 1, WRTTIDefaultAllocator<WPropertyAnimationTrack>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("ObjectPath", m_sObjectSearchSequence),
    W_MEMBER_PROPERTY("ComponentType", m_sComponentType),
    W_MEMBER_PROPERTY("Property", m_sPropertyPath),
    W_ENUM_MEMBER_PROPERTY("Target", WPropertyAnimTarget, m_Target),
    W_MEMBER_PROPERTY("FloatCurve", m_FloatCurve),
    W_MEMBER_PROPERTY("Gradient", m_ColorGradient),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WPropertyAnimationTrackGroup, 1, WRTTIDefaultAllocator<WPropertyAnimationTrackGroup>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("FPS", m_uiFramesPerSecond)->AddAttributes(new WDefaultValueAttribute(60)),
    W_MEMBER_PROPERTY("Duration", m_uiCurveDuration)->AddAttributes(new WDefaultValueAttribute(480)),
    W_ARRAY_MEMBER_PROPERTY("Tracks", m_Tracks)->AddFlags(WPropertyFlags::PointerOwner),
    W_MEMBER_PROPERTY("EventTrack", m_EventTrack),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WPropertyAnimAssetDocument, 2, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WPropertyAnimationTrackGroup::~WPropertyAnimationTrackGroup()
{
  for (WPropertyAnimationTrack* pTrack : m_Tracks)
  {
    W_DEFAULT_DELETE(pTrack);
  }
}

WPropertyAnimAssetDocument::WPropertyAnimAssetDocument(WStringView sDocumentPath)
  : WSimpleAssetDocument<WPropertyAnimationTrackGroup, WGameObjectContextDocument>(
      W_DEFAULT_NEW(WPropertyAnimObjectManager), sDocumentPath, WAssetDocEngineConnection::FullObjectMirroring)
{
  m_GameObjectContextEvents.AddEventHandler(WMakeDelegate(&WPropertyAnimAssetDocument::GameObjectContextEventHandler, this));
  m_pObjectAccessor = W_DEFAULT_NEW(WPropertyAnimObjectAccessor, this, GetCommandHistory());
}

WPropertyAnimAssetDocument::~WPropertyAnimAssetDocument()
{
  m_GameObjectContextEvents.RemoveEventHandler(WMakeDelegate(&WPropertyAnimAssetDocument::GameObjectContextEventHandler, this));

  GetObjectManager()->m_StructureEvents.RemoveEventHandler(WMakeDelegate(&WPropertyAnimAssetDocument::TreeStructureEventHandler, this));
  GetObjectManager()->m_PropertyEvents.RemoveEventHandler(WMakeDelegate(&WPropertyAnimAssetDocument::TreePropertyEventHandler, this));
}

void WPropertyAnimAssetDocument::SetAnimationDurationTicks(WUInt64 uiNumTicks)
{
  const WPropertyAnimationTrackGroup* pProp = GetProperties();

  if (pProp->m_uiCurveDuration == uiNumTicks)
    return;

  {
    WCommandHistory* history = GetCommandHistory();
    history->StartTransaction("Set Animation Duration");

    WSetObjectPropertyCommand cmdSet;
    cmdSet.m_Object = GetPropertyObject()->GetGuid();
    cmdSet.m_sProperty = "Duration";
    cmdSet.m_NewValue = uiNumTicks;
    history->AddCommand(cmdSet).AssertSuccess();

    history->FinishTransaction();
  }

  {
    WPropertyAnimAssetDocumentEvent e;
    e.m_pDocument = this;
    e.m_Type = WPropertyAnimAssetDocumentEvent::Type::AnimationLengthChanged;
    m_PropertyAnimEvents.Broadcast(e);
  }
}

WUInt64 WPropertyAnimAssetDocument::GetAnimationDurationTicks() const
{
  const WPropertyAnimationTrackGroup* pProp = GetProperties();

  return pProp->m_uiCurveDuration;
}


WTime WPropertyAnimAssetDocument::GetAnimationDurationTime() const
{
  const WInt64 ticks = GetAnimationDurationTicks();

  return WTime::MakeFromSeconds(ticks / 4800.0);
}

void WPropertyAnimAssetDocument::AdjustDuration()
{
  WUInt64 uiDuration = 480;

  const WPropertyAnimationTrackGroup* pProp = GetProperties();

  for (WUInt32 i = 0; i < pProp->m_Tracks.GetCount(); ++i)
  {
    const WPropertyAnimationTrack* pTrack = pProp->m_Tracks[i];

    for (const auto& cp : pTrack->m_FloatCurve.m_ControlPoints)
    {
      uiDuration = WMath::Max(uiDuration, (WUInt64)cp.m_iTick);
    }

    WUInt32 uiRgb = 0;
    WUInt32 uiAlpha = 0;
    WUInt32 uiIntensity = 0;
    pTrack->m_ColorGradient.m_Gradient.GetNumControlPoints(uiRgb, uiAlpha, uiIntensity);

    for (WUInt32 i = 0; i < uiRgb; ++i)
    {
      const auto& cp = pTrack->m_ColorGradient.m_Gradient.GetColorControlPoint(i);
      uiDuration = WMath::Max<WInt64>(uiDuration, cp.m_iTick);
    }

    for (WUInt32 i = 0; i < uiAlpha; ++i)
    {
      const auto& cp = pTrack->m_ColorGradient.m_Gradient.GetAlphaControlPoint(i);
      uiDuration = WMath::Max<WInt64>(uiDuration, cp.m_iTick);
    }

    for (WUInt32 i = 0; i < uiIntensity; ++i)
    {
      const auto& cp = pTrack->m_ColorGradient.m_Gradient.GetIntensityControlPoint(i);
      uiDuration = WMath::Max<WInt64>(uiDuration, cp.m_iTick);
    }
  }

  SetAnimationDurationTicks(uiDuration);
}

bool WPropertyAnimAssetDocument::SetScrubberPosition(WUInt64 uiTick)
{
  if (!m_bPlayAnimation)
  {
    const WUInt32 uiTicksPerFrame = 4800 / GetProperties()->m_uiFramesPerSecond;
    uiTick = (WUInt64)WMath::RoundToMultiple((double)uiTick, (double)uiTicksPerFrame);
  }
  uiTick = WMath::Clamp<WUInt64>(uiTick, 0, GetAnimationDurationTicks());

  if (m_uiScrubberTickPos == uiTick)
    return false;

  m_uiScrubberTickPos = uiTick;
  ApplyAnimation();

  WPropertyAnimAssetDocumentEvent e;
  e.m_pDocument = this;
  e.m_Type = WPropertyAnimAssetDocumentEvent::Type::ScrubberPositionChanged;
  m_PropertyAnimEvents.Broadcast(e);

  return true;
}

WTransformStatus WPropertyAnimAssetDocument::InternalTransformAsset(WStreamWriter& stream, WStringView sOutputTag, const WPlatformProfile* pAssetProfile,
  const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags)
{
  const WPropertyAnimationTrackGroup* pProp = GetProperties();

  WPropertyAnimResourceDescriptor desc;
  desc.m_AnimationDuration = GetAnimationDurationTime();

  for (WUInt32 i = 0; i < pProp->m_Tracks.GetCount(); ++i)
  {
    const WPropertyAnimationTrack* pTrack = pProp->m_Tracks[i];

    if (pTrack->m_Target == WPropertyAnimTarget::Color)
    {
      auto& anim = desc.m_ColorAnimations.ExpandAndGetRef();
      anim.m_sObjectSearchSequence = pTrack->m_sObjectSearchSequence;
      anim.m_sComponentType = pTrack->m_sComponentType;
      anim.m_sPropertyPath = pTrack->m_sPropertyPath;
      anim.m_Target = pTrack->m_Target;
      pTrack->m_ColorGradient.FillGradientData(anim.m_Gradient);
    }
    else
    {
      auto& anim = desc.m_FloatAnimations.ExpandAndGetRef();
      anim.m_sObjectSearchSequence = pTrack->m_sObjectSearchSequence;
      anim.m_sComponentType = pTrack->m_sComponentType;
      anim.m_sPropertyPath = pTrack->m_sPropertyPath;
      anim.m_Target = pTrack->m_Target;
      pTrack->m_FloatCurve.ConvertToRuntimeData(anim.m_Curve);
      anim.m_Curve.SortControlPoints();
      anim.m_Curve.ApplyTangentModes();
      anim.m_Curve.ClampTangents();
    }
  }

  // sort animation tracks by object path for better cache reuse at runtime
  {
    desc.m_FloatAnimations.Sort([](const WFloatPropertyAnimEntry& lhs, const WFloatPropertyAnimEntry& rhs) -> bool
      {
      const WInt32 res = lhs.m_sObjectSearchSequence.Compare(rhs.m_sObjectSearchSequence);
      if (res < 0)
        return true;
      if (res > 0)
        return false;

      return lhs.m_sComponentType < rhs.m_sComponentType; });

    desc.m_ColorAnimations.Sort([](const WColorPropertyAnimEntry& lhs, const WColorPropertyAnimEntry& rhs) -> bool
      {
      const WInt32 res = lhs.m_sObjectSearchSequence.Compare(rhs.m_sObjectSearchSequence);
      if (res < 0)
        return true;
      if (res > 0)
        return false;

      return lhs.m_sComponentType < rhs.m_sComponentType; });
  }

  pProp->m_EventTrack.ConvertToRuntimeData(desc.m_EventTrack);

  desc.Save(stream);

  return WStatus(W_SUCCESS);
}


void WPropertyAnimAssetDocument::InitializeAfterLoading(bool bFirstTimeCreation)
{
  m_pMirror = W_DEFAULT_NEW(WIPCObjectMirrorEditor);
  // Filter needs to be set before base class init as that one sends the doc.
  // (Local mirror ignores temporaries, i.e. only mirrors the asset itself)
  m_ObjectMirror.SetFilterFunction([this](const WDocumentObject* pObject, WStringView sProperty) -> bool
    { return !static_cast<WPropertyAnimObjectManager*>(GetObjectManager())->IsTemporary(pObject, sProperty); });
  // (Remote IPC mirror only sends temporaries, i.e. the context)
  m_pMirror->SetFilterFunction([this](const WDocumentObject* pObject, WStringView sProperty) -> bool
    { return static_cast<WPropertyAnimObjectManager*>(GetObjectManager())->IsTemporary(pObject, sProperty); });
  SUPER::InitializeAfterLoading(bFirstTimeCreation);
  // Important to do these after base class init as we want our subscriptions to happen after the mirror of the base class.
  GetObjectManager()->m_StructureEvents.AddEventHandler(WMakeDelegate(&WPropertyAnimAssetDocument::TreeStructureEventHandler, this));
  GetObjectManager()->m_PropertyEvents.AddEventHandler(WMakeDelegate(&WPropertyAnimAssetDocument::TreePropertyEventHandler, this));
  // Subscribe here as otherwise base init will fire a context changed event when we are not set up yet.
  // RebuildMapping();
}

void WPropertyAnimAssetDocument::GameObjectContextEventHandler(const WGameObjectContextEvent& e)
{
  switch (e.m_Type)
  {
    case WGameObjectContextEvent::Type::ContextAboutToBeChanged:
      static_cast<WPropertyAnimObjectManager*>(GetObjectManager())->SetAllowStructureChangeOnTemporaries(true);
      break;
    case WGameObjectContextEvent::Type::ContextChanged:
      static_cast<WPropertyAnimObjectManager*>(GetObjectManager())->SetAllowStructureChangeOnTemporaries(false);
      RebuildMapping();
      break;
  }
}

void WPropertyAnimAssetDocument::TreeStructureEventHandler(const WDocumentObjectStructureEvent& e)
{
  auto pManager = static_cast<WPropertyAnimObjectManager*>(GetObjectManager());
  if (e.m_pPreviousParent && pManager->IsTemporary(e.m_pPreviousParent, e.m_sParentProperty))
    return;
  if (e.m_pNewParent && pManager->IsTemporary(e.m_pNewParent, e.m_sParentProperty))
    return;

  if (e.m_pObject->GetType() == WGetStaticRTTI<WPropertyAnimationTrack>())
  {
    switch (e.m_EventType)
    {
      case WDocumentObjectStructureEvent::Type::AfterObjectAdded:
      case WDocumentObjectStructureEvent::Type::AfterObjectMoved:
        AddTrack(e.m_pObject->GetGuid());
        return;
      case WDocumentObjectStructureEvent::Type::BeforeObjectRemoved:
      case WDocumentObjectStructureEvent::Type::BeforeObjectMoved:
        RemoveTrack(e.m_pObject->GetGuid());
        return;

      default:
        break;
    }
  }
  else
  {
    ApplyAnimation();
  }
}

void WPropertyAnimAssetDocument::TreePropertyEventHandler(const WDocumentObjectPropertyEvent& e)
{
  auto pManager = static_cast<WPropertyAnimObjectManager*>(GetObjectManager());
  if (pManager->IsTemporary(e.m_pObject))
    return;

  if (e.m_pObject->GetType() == WGetStaticRTTI<WPropertyAnimationTrack>())
  {
    if (e.m_EventType == WDocumentObjectPropertyEvent::Type::PropertySet)
    {
      RemoveTrack(e.m_pObject->GetGuid());
      AddTrack(e.m_pObject->GetGuid());
      return;
    }
  }
  else
  {
    ApplyAnimation();
  }
}

void WPropertyAnimAssetDocument::RebuildMapping()
{
  while (!m_TrackTable.IsEmpty())
  {
    RemoveTrack(m_TrackTable.GetIterator().Key());
  }
  W_ASSERT_DEBUG(m_PropertyTable.IsEmpty() && m_TrackTable.IsEmpty(), "All tracks should be removed.");

  const WAbstractProperty* pTracksProp = WGetStaticRTTI<WPropertyAnimationTrackGroup>()->FindPropertyByName("Tracks");
  W_ASSERT_DEBUG(pTracksProp, "Name of property WPropertyAnimationTrackGroup::m_Tracks has changed.");
  WTempHybridArray<WVariant, 16> values;
  m_pObjectAccessor->GetValues(GetPropertyObject(), pTracksProp, values).AssertSuccess();
  for (const WVariant& value : values)
  {
    AddTrack(value.Get<WUuid>());
  }
}

void WPropertyAnimAssetDocument::RemoveTrack(const WUuid& track)
{
  auto& keys = *m_TrackTable.GetValue(track);
  for (const WPropertyReference& key : keys)
  {
    PropertyValue& value = *m_PropertyTable.GetValue(key);
    value.m_Tracks.RemoveAndSwap(track);
    ApplyAnimation(key, value);
    if (value.m_Tracks.IsEmpty())
      m_PropertyTable.Remove(key);
  }
  m_TrackTable.Remove(track);
}

void WPropertyAnimAssetDocument::AddTrack(const WUuid& track)
{
  W_ASSERT_DEV(!m_TrackTable.Contains(track), "Track already exists.");
  auto& keys = m_TrackTable[track];
  const WDocumentObject* pContext = GetContextObject();
  if (!pContext)
    return;

  auto pTrack = GetTrack(track);
  FindTrackKeys(pTrack->m_sObjectSearchSequence.GetData(), pTrack->m_sComponentType.GetData(), pTrack->m_sPropertyPath.GetData(), keys).IgnoreResult();

  for (const WPropertyReference& key : keys)
  {
    if (!m_PropertyTable.Contains(key))
    {
      PropertyValue value;
      W_VERIFY(m_pObjectAccessor->GetValue(GetObjectManager()->GetObject(key.m_Object), key.m_pProperty, value.m_InitialValue, key.m_Index).Succeeded(),
        "Computed key invalid, does not resolve to a value.");
      m_PropertyTable.Insert(key, value);
    }

    PropertyValue& value = *m_PropertyTable.GetValue(key);
    value.m_Tracks.PushBack(track);
    ApplyAnimation(key, value);
  }
}


WStatus WPropertyAnimAssetDocument::FindTrackKeys(const char* szObjectSearchSequence, const char* szComponentType, const char* szPropertyPath, WDynamicArray<WPropertyReference>& keys) const
{
  WObjectPropertyPathContext context = {GetContextObject(), m_pObjectAccessor.Borrow(), "TempObjects"};

  keys.Clear();
  return WObjectPropertyPath::ResolvePath(context, keys, szObjectSearchSequence, szComponentType, szPropertyPath);
}


void WPropertyAnimAssetDocument::GenerateTrackInfo(const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index,
  WStringBuilder& sObjectSearchSequence, WStringBuilder& sComponentType, WStringBuilder& sPropertyPath) const
{
  WObjectPropertyPathContext context = {GetContextObject(), m_pObjectAccessor.Borrow(), "TempObjects"};
  WPropertyReference propertyRef = {pObject->GetGuid(), pProp, index};
  WObjectPropertyPath::CreatePath(context, propertyRef, sObjectSearchSequence, sComponentType, sPropertyPath).AssertSuccess();
}

void WPropertyAnimAssetDocument::ApplyAnimation()
{
  for (auto it = m_PropertyTable.GetIterator(); it.IsValid(); ++it)
  {
    ApplyAnimation(it.Key(), it.Value());
  }
}

void WPropertyAnimAssetDocument::ApplyAnimation(const WPropertyReference& key, const PropertyValue& value)
{
  WVariant animValue = value.m_InitialValue;
  WAngle euler[3];
  bool bIsRotation = false;

  for (const WUuid& track : value.m_Tracks)
  {
    auto pTrack = GetTrack(track);
    const WRTTI* pPropRtti = key.m_pProperty->GetSpecificType();

    // #TODO apply pTrack to animValue
    switch (pTrack->m_Target)
    {
      case WPropertyAnimTarget::Number:
      {
        if (pPropRtti->GetVariantType() >= WVariantType::Bool && pPropRtti->GetVariantType() <= WVariantType::Double)
        {
          WVariant value2 = pTrack->m_FloatCurve.Evaluate(m_uiScrubberTickPos);
          animValue = value2.ConvertTo(animValue.GetType());
        }
      }
      break;

      case WPropertyAnimTarget::VectorX:
      case WPropertyAnimTarget::VectorY:
      case WPropertyAnimTarget::VectorZ:
      case WPropertyAnimTarget::VectorW:
      {
        if (pPropRtti->GetVariantType() >= WVariantType::Vector2 && pPropRtti->GetVariantType() <= WVariantType::Vector4U)
        {
          const double fValue = pTrack->m_FloatCurve.Evaluate(m_uiScrubberTickPos);

          WReflectionUtils::SetComponent(animValue, (WUInt32)pTrack->m_Target - WPropertyAnimTarget::VectorX, fValue);
        }
      }
      break;

      case WPropertyAnimTarget::RotationX:
      case WPropertyAnimTarget::RotationY:
      case WPropertyAnimTarget::RotationZ:
      {
        if (pPropRtti->GetVariantType() == WVariantType::Quaternion)
        {
          bIsRotation = true;
          const double fValue = pTrack->m_FloatCurve.Evaluate(m_uiScrubberTickPos);

          euler[(WUInt32)pTrack->m_Target - WPropertyAnimTarget::RotationX] = WAngle::MakeFromDegree(fValue);
        }
      }
      break;

      case WPropertyAnimTarget::Color:
      {
        if (pPropRtti->GetVariantType() == WVariantType::Color || pPropRtti->GetVariantType() == WVariantType::ColorGamma)
        {
          WVariant value2 = pTrack->m_ColorGradient.Evaluate(m_uiScrubberTickPos);
          animValue = value2.ConvertTo(animValue.GetType());
        }
      }
      break;
    }
  }

  if (bIsRotation)
  {
    WQuat qRotation;
    qRotation = WQuat::MakeFromEulerAngles(euler[0], euler[1], euler[2]);
    animValue = qRotation;
  }

  WDocumentObject* pObj = GetObjectManager()->GetObject(key.m_Object);
  WVariant oldValue;
  W_VERIFY(m_pObjectAccessor->GetValue(pObj, key.m_pProperty, oldValue, key.m_Index).Succeeded(), "Retrieving old value failed.");

  if (oldValue != animValue)
    GetObjectManager()->SetValue(pObj, key.m_pProperty->GetPropertyName(), animValue, key.m_Index).AssertSuccess();

  // tell the gizmos and manipulators that they should update their transform
  // usually they listen to the command history and selection events, but in this case no commands are executed
  {
    WGameObjectEvent e;
    e.m_Type = WGameObjectEvent::Type::GizmoTransformMayBeInvalid;
    m_GameObjectEvents.Broadcast(e);
  }
}

void WPropertyAnimAssetDocument::SetPlayAnimation(bool bPlay)
{
  if (m_bPlayAnimation == bPlay)
    return;

  if (m_uiScrubberTickPos >= GetAnimationDurationTicks())
    m_uiScrubberTickPos = 0;

  m_bPlayAnimation = bPlay;
  if (!m_bPlayAnimation)
  {
    // During playback we do not round to frames, so we need to round it again on stop.
    SetScrubberPosition(GetScrubberPosition());
  }
  m_LastFrameTime = WTime::Now();

  WPropertyAnimAssetDocumentEvent e;
  e.m_pDocument = this;
  e.m_Type = WPropertyAnimAssetDocumentEvent::Type::PlaybackChanged;
  m_PropertyAnimEvents.Broadcast(e);
}

void WPropertyAnimAssetDocument::SetRepeatAnimation(bool bRepeat)
{
  if (m_bRepeatAnimation == bRepeat)
    return;

  m_bRepeatAnimation = bRepeat;

  WPropertyAnimAssetDocumentEvent e;
  e.m_pDocument = this;
  e.m_Type = WPropertyAnimAssetDocumentEvent::Type::PlaybackChanged;
  m_PropertyAnimEvents.Broadcast(e);
}

void WPropertyAnimAssetDocument::ExecuteAnimationPlaybackStep()
{
  const WTime currentTime = WTime::Now();
  const WTime tDiff = (currentTime - m_LastFrameTime) * GetSimulationSpeed();
  const WUInt64 uiTicks = (WUInt64)(tDiff.GetSeconds() * 4800.0);
  // Accumulate further if we render too fast and round ticks to zero.
  if (uiTicks == 0)
    return;

  m_LastFrameTime = currentTime;
  const WUInt64 uiNewPos = GetScrubberPosition() + uiTicks;
  SetScrubberPosition(uiNewPos);

  if (uiNewPos > GetAnimationDurationTicks())
  {
    SetPlayAnimation(false);

    if (m_bRepeatAnimation)
      SetPlayAnimation(true);
  }
}

const WPropertyAnimationTrack* WPropertyAnimAssetDocument::GetTrack(const WUuid& track) const
{
  return const_cast<WPropertyAnimAssetDocument*>(this)->GetTrack(track);
}

WPropertyAnimationTrack* WPropertyAnimAssetDocument::GetTrack(const WUuid& track)
{
  auto obj = m_Context.GetObjectByGUID(track);
  W_ASSERT_DEBUG(obj.m_pType == WGetStaticRTTI<WPropertyAnimationTrack>(),
    "Track guid does not resolve to a track, "
    "either the track is not yet created in the mirror or already destroyed. Make sure callbacks are executed in the right order.");
  auto pTrack = static_cast<WPropertyAnimationTrack*>(obj.m_pObject);
  return pTrack;
}


WStatus WPropertyAnimAssetDocument::CanAnimate(
  const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index, WPropertyAnimTarget::Enum target) const
{
  if (!pObject)
    return WStatus("Object is null.");
  if (!pProp)
    return WStatus("Property is null.");
  if (index.IsValid())
    return WStatus("Property indices not supported.");

  if (!GetContextObject())
    return WStatus("No context set.");

  {
    const WDocumentObject* pNode = pObject;
    while (pNode && pNode != GetContextObject())
    {
      pNode = pNode->GetParent();
    }
    if (!pNode)
    {
      return WStatus("Object not below context sub-tree.");
    }
  }
  WPropertyReference key;
  key.m_Object = pObject->GetGuid();
  key.m_pProperty = pProp;
  key.m_Index = index;

  WStringBuilder sObjectSearchSequence;
  WStringBuilder sComponentType;
  WStringBuilder sPropertyPath;
  GenerateTrackInfo(pObject, pProp, index, sObjectSearchSequence, sComponentType, sPropertyPath);

  const WAbstractProperty* pName = WGetStaticRTTI<WGameObject>()->FindPropertyByName("Name");
  const WDocumentObject* pNode = pObject;
  while (pNode != GetContextObject() && pNode->GetType() != WGetStaticRTTI<WGameObject>())
  {
    pNode = pNode->GetParent();
  }
  WString sName = m_pObjectAccessor->Get<WString>(pNode, pName);

  if (sName.IsEmpty() && pNode != GetContextObject())
  {
    return WStatus("Empty node name only allowed on context root object animations.");
  }

  WTempHybridArray<WPropertyReference, 1> keys;
  return FindTrackKeys(sObjectSearchSequence.GetData(), sComponentType.GetData(), sPropertyPath.GetData(), keys);
}

WUuid WPropertyAnimAssetDocument::FindTrack(
  const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index, WPropertyAnimTarget::Enum target) const
{
  WPropertyReference key;
  key.m_Object = pObject->GetGuid();
  key.m_pProperty = pProp;
  key.m_Index = index;
  if (const PropertyValue* value = m_PropertyTable.GetValue(key))
  {
    for (const WUuid& track : value->m_Tracks)
    {
      auto pTrack = GetTrack(track);
      if (pTrack->m_Target == target)
        return track;
    }
  }
  return WUuid();
}

static WColorGammaUB g_CurveColors[10][3] = {
  {WColorGammaUB(255, 102, 0), WColorGammaUB(76, 255, 0), WColorGammaUB(0, 255, 255)},
  {WColorGammaUB(239, 35, 0), WColorGammaUB(127, 255, 0), WColorGammaUB(0, 0, 255)},
  {WColorGammaUB(205, 92, 92), WColorGammaUB(120, 158, 39), WColorGammaUB(81, 120, 188)},
  {WColorGammaUB(255, 105, 180), WColorGammaUB(0, 250, 154), WColorGammaUB(0, 191, 255)},
  {WColorGammaUB(220, 20, 60), WColorGammaUB(0, 255, 127), WColorGammaUB(30, 144, 255)},
  {WColorGammaUB(240, 128, 128), WColorGammaUB(60, 179, 113), WColorGammaUB(135, 206, 250)},
  {WColorGammaUB(178, 34, 34), WColorGammaUB(46, 139, 87), WColorGammaUB(65, 105, 225)},
  {WColorGammaUB(211, 122, 122), WColorGammaUB(144, 238, 144), WColorGammaUB(135, 206, 235)},
  {WColorGammaUB(219, 112, 147), WColorGammaUB(0, 128, 0), WColorGammaUB(70, 130, 180)},
  {WColorGammaUB(255, 182, 193), WColorGammaUB(102, 205, 170), WColorGammaUB(100, 149, 237)},
};

static WColorGammaUB g_FloatColors[10] = {
  WColorGammaUB(138, 43, 226),
  WColorGammaUB(139, 0, 139),
  WColorGammaUB(153, 50, 204),
  WColorGammaUB(148, 0, 211),
  WColorGammaUB(218, 112, 214),
  WColorGammaUB(221, 160, 221),
  WColorGammaUB(128, 0, 128),
  WColorGammaUB(102, 51, 153),
  WColorGammaUB(106, 90, 205),
  WColorGammaUB(238, 130, 238),
};

WUuid WPropertyAnimAssetDocument::CreateTrack(
  const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index, WPropertyAnimTarget::Enum target)
{
  WStringBuilder sObjectSearchSequence;
  WStringBuilder sComponentType;
  WStringBuilder sPropertyPath;
  GenerateTrackInfo(pObject, pProp, index, sObjectSearchSequence, sComponentType, sPropertyPath);

  WObjectCommandAccessor accessor(GetCommandHistory());
  const WRTTI* pTrackType = WGetStaticRTTI<WPropertyAnimationTrack>();
  WUuid newTrack;
  W_VERIFY(
    accessor.AddObject(GetPropertyObject(), WGetStaticRTTI<WPropertyAnimationTrackGroup>()->FindPropertyByName("Tracks"), -1, pTrackType, newTrack)
      .Succeeded(),
    "Adding track failed.");
  const WDocumentObject* pTrackObj = accessor.GetObject(newTrack);
  WVariant value = sObjectSearchSequence.GetData();
  W_VERIFY(accessor.SetValue(pTrackObj, pTrackType->FindPropertyByName("ObjectPath"), value).Succeeded(), "Adding track failed.");
  value = sComponentType.GetData();
  W_VERIFY(accessor.SetValue(pTrackObj, pTrackType->FindPropertyByName("ComponentType"), value).Succeeded(), "Adding track failed.");
  value = sPropertyPath.GetData();
  W_VERIFY(accessor.SetValue(pTrackObj, pTrackType->FindPropertyByName("Property"), value).Succeeded(), "Adding track failed.");
  value = (int)target;
  W_VERIFY(accessor.SetValue(pTrackObj, pTrackType->FindPropertyByName("Target"), value).Succeeded(), "Adding track failed.");

  {
    const WAbstractProperty* pFloatCurveProp = pTrackType->FindPropertyByName("FloatCurve");
    WUuid floatCurveGuid = accessor.Get<WUuid>(pTrackObj, pFloatCurveProp);
    const WDocumentObject* pFloatCurveObject = GetObjectManager()->GetObject(floatCurveGuid);

    const WAbstractProperty* pColorProp = WGetStaticRTTI<WSingleCurveData>()->FindPropertyByName("Color");

    WColorGammaUB color = WColor::White;

    const WUInt32 uiNameHash = WHashingUtils::xxHash32(sObjectSearchSequence.GetData(), sObjectSearchSequence.GetElementCount());
    const WUInt32 uiColorIdx = uiNameHash % W_ARRAY_SIZE(g_CurveColors);

    switch (target)
    {
      case WPropertyAnimTarget::Number:
        color = g_FloatColors[uiColorIdx];
        break;
      case WPropertyAnimTarget::VectorX:
      case WPropertyAnimTarget::RotationX:
        color = g_CurveColors[uiColorIdx][0];
        break;
      case WPropertyAnimTarget::VectorY:
      case WPropertyAnimTarget::RotationY:
        color = g_CurveColors[uiColorIdx][1];
        break;
      case WPropertyAnimTarget::VectorZ:
      case WPropertyAnimTarget::RotationZ:
        color = g_CurveColors[uiColorIdx][2];
        break;
      case WPropertyAnimTarget::VectorW:
        color = WColor::Beige;
        break;
      default:
        break;
    }

    accessor.SetValue(pFloatCurveObject, pColorProp, color).AssertSuccess();
  }

  return newTrack;
}

WUuid WPropertyAnimAssetDocument::FindCurveCp(const WUuid& trackGuid, WInt64 iTickX)
{
  auto pTrack = GetTrack(trackGuid);
  WInt32 iIndex = -1;
  for (WUInt32 i = 0; i < pTrack->m_FloatCurve.m_ControlPoints.GetCount(); i++)
  {
    if (pTrack->m_FloatCurve.m_ControlPoints[i].m_iTick == iTickX)
    {
      iIndex = (WInt32)i;
      break;
    }
  }
  if (iIndex == -1)
    return WUuid();

  const WAbstractProperty* pCurveProp = WGetStaticRTTI<WPropertyAnimationTrack>()->FindPropertyByName("FloatCurve");
  const WDocumentObject* trackObject = GetObjectManager()->GetObject(trackGuid);
  WUuid curveGuid = m_pObjectAccessor->Get<WUuid>(trackObject, pCurveProp);
  const WAbstractProperty* pControlPointsProp = WGetStaticRTTI<WSingleCurveData>()->FindPropertyByName("ControlPoints");
  const WDocumentObject* curveObject = GetObjectManager()->GetObject(curveGuid);
  WUuid cpGuid = m_pObjectAccessor->Get<WUuid>(curveObject, pControlPointsProp, iIndex);
  return cpGuid;
}

WUuid WPropertyAnimAssetDocument::InsertCurveCpAt(const WUuid& track, WInt64 iTickX, double fNewPosY)
{
  WObjectCommandAccessor accessor(GetCommandHistory());
  WObjectAccessorBase& acc = accessor;
  acc.StartTransaction("Insert Control Point");

  const WDocumentObject* trackObject = GetObjectManager()->GetObject(track);
  const WVariant curveGuid = trackObject->GetTypeAccessor().GetValue("FloatCurve");

  WUuid newObjectGuid;
  W_VERIFY(acc.AddObjectByName(accessor.GetObject(curveGuid.Get<WUuid>()), "ControlPoints", -1, WGetStaticRTTI<WCurveControlPointData>(), newObjectGuid).Succeeded(),
    "");
  auto curveCPObj = accessor.GetObject(newObjectGuid);
  W_VERIFY(acc.SetValueByName(curveCPObj, "Tick", iTickX).Succeeded(), "");
  W_VERIFY(acc.SetValueByName(curveCPObj, "Value", fNewPosY).Succeeded(), "");
  W_VERIFY(acc.SetValueByName(curveCPObj, "LeftTangent", WVec2(-0.1f, 0.0f)).Succeeded(), "");
  W_VERIFY(acc.SetValueByName(curveCPObj, "RightTangent", WVec2(+0.1f, 0.0f)).Succeeded(), "");

  acc.FinishTransaction();

  return newObjectGuid;
}

WUuid WPropertyAnimAssetDocument::FindGradientColorCp(const WUuid& trackGuid, WInt64 iTickX)
{
  auto pTrack = GetTrack(trackGuid);
  WInt32 iIndex = -1;
  WUInt32 numRgb, numAlpha, numIntensity;
  pTrack->m_ColorGradient.m_Gradient.GetNumControlPoints(numRgb, numAlpha, numIntensity);
  for (WUInt32 i = 0; i < numRgb; i++)
  {
    if (pTrack->m_ColorGradient.m_Gradient.GetColorControlPoint(i).m_iTick == iTickX)
    {
      iIndex = (WInt32)i;
      break;
    }
  }
  if (iIndex == -1)
    return WUuid();

  const WAbstractProperty* pCurveProp = WGetStaticRTTI<WPropertyAnimationTrack>()->FindPropertyByName("Gradient");
  const WDocumentObject* trackObject = GetObjectManager()->GetObject(trackGuid);
  WUuid curveGuid = m_pObjectAccessor->Get<WUuid>(trackObject, pCurveProp);
  const WDocumentObject* curveObject = GetObjectManager()->GetObject(curveGuid);

  // Get the nested Gradient object
  const WAbstractProperty* pGradientProp = WGetStaticRTTI<WColorGradientAssetData>()->FindPropertyByName("Gradient");
  WUuid gradientSubGuid = m_pObjectAccessor->Get<WUuid>(curveObject, pGradientProp);
  const WDocumentObject* gradientSubObject = GetObjectManager()->GetObject(gradientSubGuid);

  const WAbstractProperty* pControlPointsProp = WGetStaticRTTI<WColorGradient>()->FindPropertyByName("ColorCPs");
  WUuid cpGuid = m_pObjectAccessor->Get<WUuid>(gradientSubObject, pControlPointsProp, iIndex);
  return cpGuid;
}

WUuid WPropertyAnimAssetDocument::InsertGradientColorCpAt(const WUuid& trackGuid, WInt64 iTickX, const WColorGammaUB& color)
{
  WObjectCommandAccessor accessor(GetCommandHistory());
  WObjectAccessorBase& acc = accessor;

  const WDocumentObject* trackObject = GetObjectManager()->GetObject(trackGuid);
  const WUuid gradientGuid = trackObject->GetTypeAccessor().GetValue("Gradient").Get<WUuid>();
  const WDocumentObject* gradientObject = GetObjectManager()->GetObject(gradientGuid);

  acc.StartTransaction("Add Color Control Point");

  // Get the Gradient sub-object
  const WUuid gradientSubGuid = gradientObject->GetTypeAccessor().GetValue("Gradient").Get<WUuid>();
  const WDocumentObject* gradientSubObject = GetObjectManager()->GetObject(gradientSubGuid);

  WUuid newObjectGuid;
  W_VERIFY(acc.AddObjectByName(gradientSubObject, "ColorCPs", -1, WGetStaticRTTI<WColorGradientColorCP>(), newObjectGuid).Succeeded(), "");
  const WDocumentObject* cpObject = GetObjectManager()->GetObject(newObjectGuid);
  W_VERIFY(acc.SetValueByName(cpObject, "Tick", iTickX).Succeeded(), "");
  W_VERIFY(acc.SetValueByName(cpObject, "Red", color.r).Succeeded(), "");
  W_VERIFY(acc.SetValueByName(cpObject, "Green", color.g).Succeeded(), "");
  W_VERIFY(acc.SetValueByName(cpObject, "Blue", color.b).Succeeded(), "");
  acc.FinishTransaction();
  return newObjectGuid;
}

WUuid WPropertyAnimAssetDocument::FindGradientAlphaCp(const WUuid& trackGuid, WInt64 iTickX)
{
  auto pTrack = GetTrack(trackGuid);
  WInt32 iIndex = -1;
  WUInt32 numRgb, numAlpha, numIntensity;
  pTrack->m_ColorGradient.m_Gradient.GetNumControlPoints(numRgb, numAlpha, numIntensity);
  for (WUInt32 i = 0; i < numAlpha; i++)
  {
    if (pTrack->m_ColorGradient.m_Gradient.GetAlphaControlPoint(i).m_iTick == iTickX)
    {
      iIndex = (WInt32)i;
      break;
    }
  }
  if (iIndex == -1)
    return WUuid();

  const WAbstractProperty* pCurveProp = WGetStaticRTTI<WPropertyAnimationTrack>()->FindPropertyByName("Gradient");
  const WDocumentObject* trackObject = GetObjectManager()->GetObject(trackGuid);
  WUuid curveGuid = m_pObjectAccessor->Get<WUuid>(trackObject, pCurveProp);
  const WDocumentObject* curveObject = GetObjectManager()->GetObject(curveGuid);

  // Get the nested Gradient object
  const WAbstractProperty* pGradientProp = WGetStaticRTTI<WColorGradientAssetData>()->FindPropertyByName("Gradient");
  WUuid gradientSubGuid = m_pObjectAccessor->Get<WUuid>(curveObject, pGradientProp);
  const WDocumentObject* gradientSubObject = GetObjectManager()->GetObject(gradientSubGuid);

  const WAbstractProperty* pControlPointsProp = WGetStaticRTTI<WColorGradient>()->FindPropertyByName("AlphaCPs");
  WUuid cpGuid = m_pObjectAccessor->Get<WUuid>(gradientSubObject, pControlPointsProp, iIndex);
  return cpGuid;
}

WUuid WPropertyAnimAssetDocument::InsertGradientAlphaCpAt(const WUuid& trackGuid, WInt64 iTickX, WUInt8 uiAlpha)
{
  WObjectCommandAccessor accessor(GetCommandHistory());
  WObjectAccessorBase& acc = accessor;

  const WDocumentObject* trackObject = GetObjectManager()->GetObject(trackGuid);
  const WUuid gradientGuid = trackObject->GetTypeAccessor().GetValue("Gradient").Get<WUuid>();
  const WDocumentObject* gradientObject = GetObjectManager()->GetObject(gradientGuid);

  acc.StartTransaction("Add Alpha Control Point");

  // Get the Gradient sub-object
  const WUuid gradientSubGuid = gradientObject->GetTypeAccessor().GetValue("Gradient").Get<WUuid>();
  const WDocumentObject* gradientSubObject = GetObjectManager()->GetObject(gradientSubGuid);

  WUuid newObjectGuid;
  W_VERIFY(acc.AddObjectByName(gradientSubObject, "AlphaCPs", -1, WGetStaticRTTI<WColorGradientAlphaCP>(), newObjectGuid).Succeeded(), "");
  const WDocumentObject* cpObject = GetObjectManager()->GetObject(newObjectGuid);
  W_VERIFY(acc.SetValueByName(cpObject, "Tick", iTickX).Succeeded(), "");
  W_VERIFY(acc.SetValueByName(cpObject, "Alpha", uiAlpha).Succeeded(), "");
  acc.FinishTransaction();
  return newObjectGuid;
}

WUuid WPropertyAnimAssetDocument::FindGradientIntensityCp(const WUuid& trackGuid, WInt64 iTickX)
{
  auto pTrack = GetTrack(trackGuid);
  WInt32 iIndex = -1;
  WUInt32 numRgb, numAlpha, numIntensity;
  pTrack->m_ColorGradient.m_Gradient.GetNumControlPoints(numRgb, numAlpha, numIntensity);
  for (WUInt32 i = 0; i < numIntensity; i++)
  {
    if (pTrack->m_ColorGradient.m_Gradient.GetIntensityControlPoint(i).m_iTick == iTickX)
    {
      iIndex = (WInt32)i;
      break;
    }
  }
  if (iIndex == -1)
    return WUuid();

  const WAbstractProperty* pCurveProp = WGetStaticRTTI<WPropertyAnimationTrack>()->FindPropertyByName("Gradient");
  const WDocumentObject* trackObject = GetObjectManager()->GetObject(trackGuid);
  WUuid curveGuid = m_pObjectAccessor->Get<WUuid>(trackObject, pCurveProp);
  const WDocumentObject* curveObject = GetObjectManager()->GetObject(curveGuid);

  // Get the nested Gradient object
  const WAbstractProperty* pGradientProp = WGetStaticRTTI<WColorGradientAssetData>()->FindPropertyByName("Gradient");
  WUuid gradientSubGuid = m_pObjectAccessor->Get<WUuid>(curveObject, pGradientProp);
  const WDocumentObject* gradientSubObject = GetObjectManager()->GetObject(gradientSubGuid);

  const WAbstractProperty* pControlPointsProp = WGetStaticRTTI<WColorGradient>()->FindPropertyByName("IntensityCPs");
  WUuid cpGuid = m_pObjectAccessor->Get<WUuid>(gradientSubObject, pControlPointsProp, iIndex);
  return cpGuid;
}

WUuid WPropertyAnimAssetDocument::InsertGradientIntensityCpAt(const WUuid& trackGuid, WInt64 iTickX, float fIntensity)
{
  WObjectCommandAccessor accessor(GetCommandHistory());
  WObjectAccessorBase& acc = accessor;

  const WDocumentObject* trackObject = GetObjectManager()->GetObject(trackGuid);
  const WUuid gradientGuid = trackObject->GetTypeAccessor().GetValue("Gradient").Get<WUuid>();
  const WDocumentObject* gradientObject = GetObjectManager()->GetObject(gradientGuid);

  acc.StartTransaction("Add Intensity Control Point");

  // Get the Gradient sub-object
  const WUuid gradientSubGuid = gradientObject->GetTypeAccessor().GetValue("Gradient").Get<WUuid>();
  const WDocumentObject* gradientSubObject = GetObjectManager()->GetObject(gradientSubGuid);

  WUuid newObjectGuid;
  W_VERIFY(acc.AddObjectByName(gradientSubObject, "IntensityCPs", -1, WGetStaticRTTI<WColorGradientIntensityCP>(), newObjectGuid).Succeeded(), "");
  const WDocumentObject* cpObject = GetObjectManager()->GetObject(newObjectGuid);
  W_VERIFY(acc.SetValueByName(cpObject, "Tick", iTickX).Succeeded(), "");
  W_VERIFY(acc.SetValueByName(cpObject, "Intensity", fIntensity).Succeeded(), "");
  acc.FinishTransaction();
  return newObjectGuid;
}

WUuid WPropertyAnimAssetDocument::InsertEventTrackCpAt(WInt64 iTickX, const char* szValue)
{
  WObjectCommandAccessor accessor(GetCommandHistory());
  WObjectAccessorBase& acc = accessor;
  acc.StartTransaction("Insert Event");

  const WAbstractProperty* pTrackProp = WGetStaticRTTI<WPropertyAnimationTrackGroup>()->FindPropertyByName("EventTrack");
  WUuid trackGuid = accessor.Get<WUuid>(GetPropertyObject(), pTrackProp);

  WUuid newObjectGuid;
  W_VERIFY(acc.AddObjectByName(accessor.GetObject(trackGuid), "ControlPoints", -1, WGetStaticRTTI<WEventTrackControlPointData>(), newObjectGuid).Succeeded(),
    "");
  const WDocumentObject* pCPObj = accessor.GetObject(newObjectGuid);
  W_VERIFY(acc.SetValueByName(pCPObj, "Tick", iTickX).Succeeded(), "");
  W_VERIFY(acc.SetValueByName(pCPObj, "Event", szValue).Succeeded(), "");

  acc.FinishTransaction();

  return newObjectGuid;
}
