#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimAsset.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimObjectAccessor.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimObjectManager.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WPropertyAnimObjectAccessor, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WPropertyAnimObjectAccessor::WPropertyAnimObjectAccessor(WPropertyAnimAssetDocument* pDoc, WCommandHistory* pHistory)
  : WObjectCommandAccessor(pHistory)
  , m_pDocument(pDoc)
  , m_pObjectManager(static_cast<WPropertyAnimObjectManager*>(pDoc->GetObjectManager()))
{
  m_pObjAccessor = W_DEFAULT_NEW(WObjectCommandAccessor, pHistory);
}

WStatus WPropertyAnimObjectAccessor::GetValue(
  const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant& out_value, WVariant index /*= WVariant()*/)
{
  return WObjectCommandAccessor::GetValue(pObject, pProp, out_value, index);
}

WStatus WPropertyAnimObjectAccessor::SetValue(
  const WDocumentObject* pObject, const WAbstractProperty* pProp, const WVariant& newValue, WVariant index)
{
  if (IsTemporary(pObject))
  {
    WVariant oldValue;
    W_VERIFY(m_pObjAccessor->GetValue(pObject, pProp, oldValue, index).Succeeded(), "Property does not exist, can't animate");

    WVariantType::Enum type = pProp->GetSpecificType()->GetVariantType();
    if (type >= WVariantType::Bool && type <= WVariantType::Double)
    {
      return SetCurveCp(pObject, pProp, index, WPropertyAnimTarget::Number, oldValue.ConvertTo<double>(), newValue.ConvertTo<double>());
    }
    else if (type >= WVariantType::Vector2 && type <= WVariantType::Vector4U)
    {
      const WUInt32 uiComponents = WReflectionUtils::GetComponentCount(type);
      for (WUInt32 c = 0; c < uiComponents; c++)
      {
        const double fOldValue = WReflectionUtils::GetComponent(oldValue, c);
        const double fValue = WReflectionUtils::GetComponent(newValue, c);

        if (WMath::IsEqual(fOldValue, fValue, WMath::SmallEpsilon<double>()))
          continue;

        W_SUCCEED_OR_RETURN(
          SetCurveCp(pObject, pProp, index, static_cast<WPropertyAnimTarget::Enum>((int)WPropertyAnimTarget::VectorX + c), fOldValue, fValue));
      }

      return WStatus(W_SUCCESS);
    }
    else if (type == WVariantType::Color)
    {
      auto oldColor = oldValue.Get<WColor>();
      WColorGammaUB oldColorGamma;
      WUInt8 oldAlpha;
      float oldIntensity;
      SeparateColor(oldColor, oldColorGamma, oldAlpha, oldIntensity);
      auto newColor = newValue.Get<WColor>();
      WColorGammaUB newColorGamma;
      WUInt8 newAlpha;
      float newIntensity;
      SeparateColor(newColor, newColorGamma, newAlpha, newIntensity);

      WStatus res(W_SUCCESS);
      if (oldColorGamma != newColorGamma)
      {
        res = SetColorCurveCp(pObject, pProp, index, oldColorGamma, newColorGamma);
      }
      if (oldAlpha != newAlpha && res.Succeeded())
      {
        res = SetAlphaCurveCp(pObject, pProp, index, oldAlpha, newAlpha);
      }
      if (!WMath::IsEqual(oldIntensity, newIntensity, WMath::SmallEpsilon<float>()) && res.Succeeded())
      {
        res = SetIntensityCurveCp(pObject, pProp, index, oldIntensity, newIntensity);
      }
      return res;
    }
    else if (type == WVariantType::ColorGamma)
    {
      auto oldColorGamma = oldValue.Get<WColorGammaUB>();
      WUInt8 oldAlpha = oldColorGamma.a;
      oldColorGamma.a = 255;

      auto newColorGamma = newValue.Get<WColorGammaUB>();
      WUInt8 newAlpha = newColorGamma.a;
      newColorGamma.a = 255;

      WStatus res(W_SUCCESS);
      if (oldColorGamma != newColorGamma)
      {
        res = SetColorCurveCp(pObject, pProp, index, oldColorGamma, newColorGamma);
      }
      if (oldAlpha != newAlpha && res.Succeeded())
      {
        res = SetAlphaCurveCp(pObject, pProp, index, oldAlpha, newAlpha);
      }
      return res;
    }
    else if (type == WVariantType::Quaternion)
    {

      const WQuat qOldRot = oldValue.Get<WQuat>();
      const WQuat qNewRot = newValue.Get<WQuat>();

      WAngle oldEuler[3];
      qOldRot.GetAsEulerAngles(oldEuler[0], oldEuler[1], oldEuler[2]);
      WAngle newEuler[3];
      qNewRot.GetAsEulerAngles(newEuler[0], newEuler[1], newEuler[2]);

      for (WUInt32 c = 0; c < 3; c++)
      {
        W_SUCCEED_OR_RETURN(
          m_pDocument->CanAnimate(pObject, pProp, index, static_cast<WPropertyAnimTarget::Enum>((int)WPropertyAnimTarget::RotationX + c)));
        float oldValue = oldEuler[c].GetDegree();
        WUuid track = FindOrAddTrack(pObject, pProp, index, static_cast<WPropertyAnimTarget::Enum>((int)WPropertyAnimTarget::RotationX + c),
          [this, oldValue](const WUuid& trackGuid)
          {
            // add a control point at the start of the curve with the original value
            m_pDocument->InsertCurveCpAt(trackGuid, 0, oldValue);
          });
        const auto* pTrack = m_pDocument->GetTrack(track);
        oldEuler[c] = WAngle::MakeFromDegree(pTrack->m_FloatCurve.Evaluate(m_pDocument->GetScrubberPosition()));
      }

      for (WUInt32 c = 0; c < 3; c++)
      {
        // We assume the change is less than 180 degrees from the old value
        float fDiff = (newEuler[c] - oldEuler[c]).GetDegree();
        float iRounds = WMath::RoundToMultiple(fDiff, 360.0f);
        fDiff -= iRounds;
        newEuler[c] = oldEuler[c] + WAngle::MakeFromDegree(fDiff);
        if (oldEuler[c].IsEqualSimple(newEuler[c], WAngle::MakeFromDegree(0.01f)))
          continue;

        W_SUCCEED_OR_RETURN(SetCurveCp(pObject, pProp, index, static_cast<WPropertyAnimTarget::Enum>((int)WPropertyAnimTarget::RotationX + c),
          oldEuler[c].GetDegree(), newEuler[c].GetDegree()));
      }

      return WStatus(W_SUCCESS);
    }

    return WStatus(WFmt("The property '{0}' cannot be animated.", pProp->GetPropertyName()));
  }
  else
  {
    return WObjectCommandAccessor::SetValue(pObject, pProp, newValue, index);
  }
}

WStatus WPropertyAnimObjectAccessor::InsertValue(
  const WDocumentObject* pObject, const WAbstractProperty* pProp, const WVariant& newValue, WVariant index /*= WVariant()*/)
{
  if (IsTemporary(pObject))
  {
    return WStatus("The structure of the context cannot be animated.");
  }
  else
  {
    return WObjectCommandAccessor::InsertValue(pObject, pProp, newValue, index);
  }
}

WStatus WPropertyAnimObjectAccessor::RemoveValue(
  const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index /*= WVariant()*/)
{
  if (IsTemporary(pObject))
  {
    return WStatus("The structure of the context cannot be animated.");
  }
  else
  {
    return WObjectCommandAccessor::RemoveValue(pObject, pProp, index);
  }
}

WStatus WPropertyAnimObjectAccessor::MoveValue(
  const WDocumentObject* pObject, const WAbstractProperty* pProp, const WVariant& oldIndex, const WVariant& newIndex)
{
  if (IsTemporary(pObject))
  {
    return WStatus("The structure of the context cannot be animated.");
  }
  else
  {
    return WObjectCommandAccessor::MoveValue(pObject, pProp, oldIndex, newIndex);
  }
}

WStatus WPropertyAnimObjectAccessor::AddObject(
  const WDocumentObject* pParent, const WAbstractProperty* pParentProp, const WVariant& index, const WRTTI* pType, WUuid& inout_objectGuid)
{
  if (IsTemporary(pParent, pParentProp))
  {
    return WStatus("The structure of the context cannot be animated.");
  }
  else
  {
    return WObjectCommandAccessor::AddObject(pParent, pParentProp, index, pType, inout_objectGuid);
  }
}

WStatus WPropertyAnimObjectAccessor::RemoveObject(const WDocumentObject* pObject)
{
  if (IsTemporary(pObject))
  {
    return WStatus("The structure of the context cannot be animated.");
  }
  else
  {
    return WObjectCommandAccessor::RemoveObject(pObject);
  }
}

WStatus WPropertyAnimObjectAccessor::MoveObject(
  const WDocumentObject* pObject, const WDocumentObject* pNewParent, const WAbstractProperty* pParentProp, const WVariant& index)
{
  if (IsTemporary(pObject))
  {
    return WStatus("The structure of the context cannot be animated.");
  }
  else
  {
    return WObjectCommandAccessor::MoveObject(pObject, pNewParent, pParentProp, index);
  }
}

bool WPropertyAnimObjectAccessor::IsTemporary(const WDocumentObject* pObject) const
{
  return m_pObjectManager->IsTemporary(pObject);
}

bool WPropertyAnimObjectAccessor::IsTemporary(const WDocumentObject* pParent, const WAbstractProperty* pParentProp) const
{
  return m_pObjectManager->IsTemporary(pParent, pParentProp->GetPropertyName());
}


WStatus WPropertyAnimObjectAccessor::SetCurveCp(const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index,
  WPropertyAnimTarget::Enum target, double fOldValue, double fNewValue)
{
  W_SUCCEED_OR_RETURN(m_pDocument->CanAnimate(pObject, pProp, index, target));
  WUuid track = FindOrAddTrack(pObject, pProp, index, target, [this, fOldValue](const WUuid& trackGuid)
    {
    // add a control point at the start of the curve with the original value
    m_pDocument->InsertCurveCpAt(trackGuid, 0, fOldValue); });
  return SetOrInsertCurveCp(track, fNewValue);
}

WUuid WPropertyAnimObjectAccessor::FindOrAddTrack(
  const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index, WPropertyAnimTarget::Enum target, OnAddTrack onAddTrack)
{
  WUuid track = m_pDocument->FindTrack(pObject, pProp, index, target);
  if (!track.IsValid())
  {
    auto pHistory = m_pDocument->GetCommandHistory();
    bool bWasTemporaryTransaction = pHistory->InTemporaryTransaction();
    if (bWasTemporaryTransaction)
    {
      pHistory->SuspendTemporaryTransaction();
    }
    track = m_pDocument->CreateTrack(pObject, pProp, index, target);
    onAddTrack(track);

    if (bWasTemporaryTransaction)
    {
      pHistory->ResumeTemporaryTransaction();
    }
  }
  W_ASSERT_DEBUG(track.IsValid(), "Creating track failed.");
  return track;
}

WStatus WPropertyAnimObjectAccessor::SetOrInsertCurveCp(const WUuid& track, double fValue)
{
  const WInt64 iScrubberPos = (WInt64)m_pDocument->GetScrubberPosition();
  WUuid cpGuid = m_pDocument->FindCurveCp(track, iScrubberPos);
  if (cpGuid.IsValid())
  {
    auto pCP = GetObject(cpGuid);
    W_VERIFY(m_pObjAccessor->SetValueByName(pCP, "Value", fValue).Succeeded(), "");
  }
  else
  {
    auto pHistory = m_pDocument->GetCommandHistory();
    bool bWasTemporaryTransaction = pHistory->InTemporaryTransaction();
    if (bWasTemporaryTransaction)
    {
      pHistory->SuspendTemporaryTransaction();
    }
    cpGuid = m_pDocument->InsertCurveCpAt(track, iScrubberPos, fValue);
    if (bWasTemporaryTransaction)
    {
      pHistory->ResumeTemporaryTransaction();
    }
  }
  return WStatus(W_SUCCESS);
}

WStatus WPropertyAnimObjectAccessor::SetColorCurveCp(const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index, const WColorGammaUB& oldValue, const WColorGammaUB& newValue)
{
  W_SUCCEED_OR_RETURN(m_pDocument->CanAnimate(pObject, pProp, index, WPropertyAnimTarget::Color));
  WUuid track = FindOrAddTrack(pObject, pProp, index, WPropertyAnimTarget::Color, [this, &oldValue](const WUuid& trackGuid)
    {
      // add a control point at the start of the curve with the original value
      m_pDocument->InsertGradientColorCpAt(trackGuid, 0, oldValue);
      //
    });

  W_SUCCEED_OR_RETURN(SetOrInsertColorCurveCp(track, newValue));

  return WStatus(W_SUCCESS);
}

WStatus WPropertyAnimObjectAccessor::SetOrInsertColorCurveCp(const WUuid& track, const WColorGammaUB& value)
{
  const WInt64 iScrubberPos = (WInt64)m_pDocument->GetScrubberPosition();
  WUuid cpGuid = m_pDocument->FindGradientColorCp(track, iScrubberPos);
  if (cpGuid.IsValid())
  {
    auto pCP = GetObject(cpGuid);
    W_VERIFY(m_pObjAccessor->SetValueByName(pCP, "Red", value.r).Succeeded(), "");
    W_VERIFY(m_pObjAccessor->SetValueByName(pCP, "Green", value.g).Succeeded(), "");
    W_VERIFY(m_pObjAccessor->SetValueByName(pCP, "Blue", value.b).Succeeded(), "");
  }
  else
  {
    auto pHistory = m_pDocument->GetCommandHistory();
    bool bWasTemporaryTransaction = pHistory->InTemporaryTransaction();
    if (bWasTemporaryTransaction)
    {
      pHistory->SuspendTemporaryTransaction();
    }
    cpGuid = m_pDocument->InsertGradientColorCpAt(track, iScrubberPos, value);
    if (bWasTemporaryTransaction)
    {
      pHistory->ResumeTemporaryTransaction();
    }
  }
  return WStatus(W_SUCCESS);
}

WStatus WPropertyAnimObjectAccessor::SetAlphaCurveCp(const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index, WUInt8 oldValue, WUInt8 newValue)
{
  W_SUCCEED_OR_RETURN(m_pDocument->CanAnimate(pObject, pProp, index, WPropertyAnimTarget::Color));
  WUuid track = FindOrAddTrack(pObject, pProp, index, WPropertyAnimTarget::Color, [this, &oldValue](const WUuid& trackGuid)
    {
      // add a control point at the start of the curve with the original value
      m_pDocument->InsertGradientAlphaCpAt(trackGuid, 0, oldValue);
      //
    });

  W_SUCCEED_OR_RETURN(SetOrInsertAlphaCurveCp(track, newValue));
  return WStatus(W_SUCCESS);
}

WStatus WPropertyAnimObjectAccessor::SetOrInsertAlphaCurveCp(const WUuid& track, WUInt8 value)
{
  const WInt64 iScrubberPos = (WInt64)m_pDocument->GetScrubberPosition();
  WUuid cpGuid = m_pDocument->FindGradientAlphaCp(track, iScrubberPos);
  if (cpGuid.IsValid())
  {
    auto pCP = GetObject(cpGuid);
    W_VERIFY(m_pObjAccessor->SetValueByName(pCP, "Alpha", value).Succeeded(), "");
  }
  else
  {
    auto pHistory = m_pDocument->GetCommandHistory();
    bool bWasTemporaryTransaction = pHistory->InTemporaryTransaction();
    if (bWasTemporaryTransaction)
    {
      pHistory->SuspendTemporaryTransaction();
    }
    cpGuid = m_pDocument->InsertGradientAlphaCpAt(track, iScrubberPos, value);
    if (bWasTemporaryTransaction)
    {
      pHistory->ResumeTemporaryTransaction();
    }
  }
  return WStatus(W_SUCCESS);
}

WStatus WPropertyAnimObjectAccessor::SetIntensityCurveCp(const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index, float oldValue, float newValue)
{
  WUuid track = FindOrAddTrack(pObject, pProp, index, WPropertyAnimTarget::Color, [this, &oldValue](const WUuid& trackGuid)
    {
      // add a control point at the start of the curve with the original value
      m_pDocument->InsertGradientIntensityCpAt(trackGuid, 0, oldValue);
      //
    });

  W_SUCCEED_OR_RETURN(SetOrInsertIntensityCurveCp(track, newValue));
  return WStatus(W_SUCCESS);
}

WStatus WPropertyAnimObjectAccessor::SetOrInsertIntensityCurveCp(const WUuid& track, float value)
{
  const WInt64 iScrubberPos = (WInt64)m_pDocument->GetScrubberPosition();
  WUuid cpGuid = m_pDocument->FindGradientIntensityCp(track, iScrubberPos);
  if (cpGuid.IsValid())
  {
    auto pCP = GetObject(cpGuid);
    W_VERIFY(m_pObjAccessor->SetValueByName(pCP, "Intensity", value).Succeeded(), "");
  }
  else
  {
    auto pHistory = m_pDocument->GetCommandHistory();
    bool bWasTemporaryTransaction = pHistory->InTemporaryTransaction();
    if (bWasTemporaryTransaction)
    {
      pHistory->SuspendTemporaryTransaction();
    }
    cpGuid = m_pDocument->InsertGradientIntensityCpAt(track, iScrubberPos, value);
    if (bWasTemporaryTransaction)
    {
      pHistory->ResumeTemporaryTransaction();
    }
  }
  return WStatus(W_SUCCESS);
}

void WPropertyAnimObjectAccessor::SeparateColor(const WColor& color, WColorGammaUB& gamma, WUInt8& alpha, float& intensity)
{
  alpha = static_cast<WColorGammaUB>(color).a;
  intensity = WMath::Max(color.r, color.g, color.b);
  if (intensity > 1.0f)
  {
    gamma = (color / intensity);
    gamma.a = 255;
  }
  else
  {
    intensity = 1.0f;
    gamma = color;
    gamma.a = 255;
  }
}
