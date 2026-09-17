#include <Foundation/FoundationPCH.h>

#include <Foundation/Reflection/PropertyPath.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Types/UniquePtr.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(WPropertyPathStep, WNoBase, 1, WRTTIDefaultAllocator<WPropertyPathStep>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Property", m_sProperty),
    W_MEMBER_PROPERTY("Index", m_Index),
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;
// clang-format on

WPropertyPath::WPropertyPath() = default;
WPropertyPath::~WPropertyPath() = default;

bool WPropertyPath::IsValid() const
{
  return m_bIsValid;
}

WResult WPropertyPath::InitializeFromPath(const WRTTI& rootObjectRtti, const char* szPath)
{
  m_bIsValid = false;

  const WStringBuilder sPathParts = szPath;
  WStringBuilder sIndex;
  WStringBuilder sFieldName;

  WTempHybridArray<WStringView, 4> parts;
  sPathParts.Split(false, parts, "/");

  // an empty path is valid as well

  m_PathSteps.Clear();
  m_PathSteps.Reserve(parts.GetCount());

  const WRTTI* pCurRtti = &rootObjectRtti;

  for (const WStringView& part : parts)
  {
    if (part.EndsWith("]"))
    {
      const char* szBracket = part.FindSubString("[");

      sIndex.SetSubString_FromTo(szBracket + 1, part.GetEndPointer() - 1);

      sFieldName.SetSubString_FromTo(part.GetStartPointer(), szBracket);
    }
    else
    {
      sFieldName = part;
      sIndex.Clear();
    }

    const WAbstractProperty* pAbsProp = pCurRtti->FindPropertyByName(sFieldName);

    if (pAbsProp == nullptr)
      return W_FAILURE;

    auto& step = m_PathSteps.ExpandAndGetRef();
    step.m_pProperty = pAbsProp;

    if (pAbsProp->GetCategory() == WPropertyCategory::Array)
    {
      if (sIndex.IsEmpty())
      {
        step.m_Index = WVariant();
      }
      else
      {
        WInt32 iIndex;
        W_SUCCEED_OR_RETURN(WConversionUtils::StringToInt(sIndex, iIndex));
        step.m_Index = iIndex;
      }
    }
    else if (pAbsProp->GetCategory() == WPropertyCategory::Set)
    {
      if (sIndex.IsEmpty())
      {
        step.m_Index = WVariant();
      }
      else
      {
        return W_FAILURE;
      }
    }
    else if (pAbsProp->GetCategory() == WPropertyCategory::Map)
    {
      step.m_Index = sIndex.IsEmpty() ? WVariant() : WVariant(sIndex.GetData());
    }

    pCurRtti = pAbsProp->GetSpecificType();
  }

  m_bIsValid = true;
  return W_SUCCESS;
}

WResult WPropertyPath::InitializeFromPath(const WRTTI* pRootObjectRtti, const WArrayPtr<const WPropertyPathStep> path)
{
  m_bIsValid = false;

  m_PathSteps.Clear();
  m_PathSteps.Reserve(path.GetCount());

  const WRTTI* pCurRtti = pRootObjectRtti;
  for (const WPropertyPathStep& pathStep : path)
  {
    const WAbstractProperty* pAbsProp = pCurRtti->FindPropertyByName(pathStep.m_sProperty);
    if (pAbsProp == nullptr)
      return W_FAILURE;

    auto& step = m_PathSteps.ExpandAndGetRef();
    step.m_pProperty = pAbsProp;
    step.m_Index = pathStep.m_Index;

    pCurRtti = pAbsProp->GetSpecificType();
  }

  m_bIsValid = true;
  return W_SUCCESS;
}

WResult WPropertyPath::WriteToLeafObject(void* pRootObject, const WRTTI* pType, WDelegate<void(void* pLeaf, const WRTTI& pType)> func) const
{
  W_ASSERT_DEBUG(
    m_PathSteps.IsEmpty() || m_PathSteps[m_PathSteps.GetCount() - 1].m_pProperty->GetSpecificType()->GetTypeFlags().IsSet(WTypeFlags::Class),
    "To resolve the leaf object the path needs to be empty or end in a class.");
  return ResolvePath(pRootObject, pType, m_PathSteps.GetArrayPtr(), true, func);
}

WResult WPropertyPath::ReadFromLeafObject(void* pRootObject, const WRTTI* pType, WDelegate<void(void* pLeaf, const WRTTI& pType)> func) const
{
  W_ASSERT_DEBUG(
    m_PathSteps.IsEmpty() || m_PathSteps[m_PathSteps.GetCount() - 1].m_pProperty->GetSpecificType()->GetTypeFlags().IsSet(WTypeFlags::Class),
    "To resolve the leaf object the path needs to be empty or end in a class.");
  return ResolvePath(pRootObject, pType, m_PathSteps.GetArrayPtr(), false, func);
}

WResult WPropertyPath::WriteProperty(
  void* pRootObject, const WRTTI& type, WDelegate<void(void* pLeafObject, const WRTTI& pLeafType, const WAbstractProperty* pProp, const WVariant& index)> func) const
{
  W_ASSERT_DEBUG(!m_PathSteps.IsEmpty(), "Call InitializeFromPath before WriteToObject");
  return ResolvePath(pRootObject, &type, m_PathSteps.GetArrayPtr().GetSubArray(0, m_PathSteps.GetCount() - 1), true,
    [this, &func](void* pLeafObject, const WRTTI& leafType)
    {
      auto& lastStep = m_PathSteps[m_PathSteps.GetCount() - 1];
      func(pLeafObject, leafType, lastStep.m_pProperty, lastStep.m_Index);
    });
}

WResult WPropertyPath::ReadProperty(
  void* pRootObject, const WRTTI& type, WDelegate<void(void* pLeafObject, const WRTTI& pLeafType, const WAbstractProperty* pProp, const WVariant& index)> func) const
{
  W_ASSERT_DEBUG(m_bIsValid, "Call InitializeFromPath before WriteToObject");
  return ResolvePath(pRootObject, &type, m_PathSteps.GetArrayPtr().GetSubArray(0, m_PathSteps.GetCount() - 1), false,
    [this, &func](void* pLeafObject, const WRTTI& leafType)
    {
      auto& lastStep = m_PathSteps[m_PathSteps.GetCount() - 1];
      func(pLeafObject, leafType, lastStep.m_pProperty, lastStep.m_Index);
    });
}

void WPropertyPath::SetValue(void* pRootObject, const WRTTI& type, const WVariant& value) const
{
  // W_ASSERT_DEBUG(!m_PathSteps.IsEmpty() &&
  //                    value.CanConvertTo(m_PathSteps[m_PathSteps.GetCount() - 1].m_pProperty->GetSpecificType()->GetVariantType()),
  //                "The given value does not match the type at the given path.");

  WriteProperty(pRootObject, type, [&value](void* pLeaf, const WRTTI& type, const WAbstractProperty* pProp, const WVariant& index)
    {
      W_IGNORE_UNUSED(type);

    switch (pProp->GetCategory())
    {
      case WPropertyCategory::Member:
        WReflectionUtils::SetMemberPropertyValue(static_cast<const WAbstractMemberProperty*>(pProp), pLeaf, value);
        break;
      case WPropertyCategory::Array:
        WReflectionUtils::SetArrayPropertyValue(static_cast<const WAbstractArrayProperty*>(pProp), pLeaf, index.Get<WInt32>(), value);
        break;
      case WPropertyCategory::Map:
        WReflectionUtils::SetMapPropertyValue(static_cast<const WAbstractMapProperty*>(pProp), pLeaf, index.Get<WString>(), value);
        break;
      default:
        W_ASSERT_NOT_IMPLEMENTED;
        break;
    } })
    .IgnoreResult();
}

void WPropertyPath::GetValue(void* pRootObject, const WRTTI& type, WVariant& out_value) const
{
  // W_ASSERT_DEBUG(!m_PathSteps.IsEmpty() &&
  //                    m_PathSteps[m_PathSteps.GetCount() - 1].m_pProperty->GetSpecificType()->GetVariantType() != WVariantType::Invalid,
  //                "The property path of value {} cannot be stored in an WVariant.", m_PathSteps[m_PathSteps.GetCount() -
  //                1].m_pProperty->GetSpecificType()->GetTypeName());

  ReadProperty(pRootObject, type, [&out_value](void* pLeaf, const WRTTI& type, const WAbstractProperty* pProp, const WVariant& index)
    {
      W_IGNORE_UNUSED(type);

    switch (pProp->GetCategory())
    {
      case WPropertyCategory::Member:
        out_value = WReflectionUtils::GetMemberPropertyValue(static_cast<const WAbstractMemberProperty*>(pProp), pLeaf);
        break;
      case WPropertyCategory::Array:
        out_value = WReflectionUtils::GetArrayPropertyValue(static_cast<const WAbstractArrayProperty*>(pProp), pLeaf, index.Get<WInt32>());
        break;
      case WPropertyCategory::Map:
        out_value = WReflectionUtils::GetMapPropertyValue(static_cast<const WAbstractMapProperty*>(pProp), pLeaf, index.Get<WString>());
        break;
      default:
        W_ASSERT_NOT_IMPLEMENTED;
        break;
    } })
    .IgnoreResult();
}

WResult WPropertyPath::ResolvePath(void* pCurrentObject, const WRTTI* pType, const WArrayPtr<const ResolvedStep> path, bool bWriteToObject,
  const WDelegate<void(void* pLeaf, const WRTTI& pType)>& func)
{
  if (path.IsEmpty())
  {
    func(pCurrentObject, *pType);
    return W_SUCCESS;
  }
  else // Recurse
  {
    const WAbstractProperty* pProp = path[0].m_pProperty;
    const WRTTI* pPropType = pProp->GetSpecificType();

    switch (pProp->GetCategory())
    {
      case WPropertyCategory::Member:
      {
        auto pSpecific = static_cast<const WAbstractMemberProperty*>(pProp);
        if (pPropType->GetProperties().GetCount() > 0)
        {
          void* pSubObject = pSpecific->GetPropertyPointer(pCurrentObject);
          // Do we have direct access to the property?
          if (pSubObject != nullptr)
          {
            return ResolvePath(pSubObject, pProp->GetSpecificType(), path.GetSubArray(1), bWriteToObject, func);
          }
          // If the property is behind an accessor, we need to retrieve it first.
          else if (pPropType->GetAllocator()->CanAllocate())
          {
            void* pRetrievedSubObject = pPropType->GetAllocator()->Allocate<void>();
            pSpecific->GetValuePtr(pCurrentObject, pRetrievedSubObject);

            WResult res = ResolvePath(pRetrievedSubObject, pProp->GetSpecificType(), path.GetSubArray(1), bWriteToObject, func);

            if (bWriteToObject)
              pSpecific->SetValuePtr(pCurrentObject, pRetrievedSubObject);

            pPropType->GetAllocator()->Deallocate(pRetrievedSubObject);
            return res;
          }
          else
          {
            W_REPORT_FAILURE("Non-allocatable property should not be part of an object chain!");
          }
        }
      }
      break;
      case WPropertyCategory::Array:
      {
        auto pSpecific = static_cast<const WAbstractArrayProperty*>(pProp);

        if (pPropType->GetAllocator()->CanAllocate())
        {
          const WUInt32 uiIndex = path[0].m_Index.ConvertTo<WUInt32>();
          if (uiIndex >= pSpecific->GetCount(pCurrentObject))
            return W_FAILURE;

          void* pSubObject = pPropType->GetAllocator()->Allocate<void>();
          pSpecific->GetValue(pCurrentObject, uiIndex, pSubObject);

          WResult res = ResolvePath(pSubObject, pProp->GetSpecificType(), path.GetSubArray(1), bWriteToObject, func);

          if (bWriteToObject)
            pSpecific->SetValue(pCurrentObject, uiIndex, pSubObject);

          pPropType->GetAllocator()->Deallocate(pSubObject);
          return res;
        }
        else
        {
          W_REPORT_FAILURE("Non-allocatable property should not be part of an object chain!");
        }
      }
      break;
      case WPropertyCategory::Map:
      {
        auto pSpecific = static_cast<const WAbstractMapProperty*>(pProp);
        const WString& sKey = path[0].m_Index.Get<WString>();
        if (!pSpecific->Contains(pCurrentObject, sKey))
          return W_FAILURE;

        if (pPropType->GetAllocator()->CanAllocate())
        {
          void* pSubObject = pPropType->GetAllocator()->Allocate<void>();

          pSpecific->GetValue(pCurrentObject, sKey, pSubObject);

          WResult res = ResolvePath(pSubObject, pProp->GetSpecificType(), path.GetSubArray(1), bWriteToObject, func);

          if (bWriteToObject)
            pSpecific->Insert(pCurrentObject, sKey, pSubObject);

          pPropType->GetAllocator()->Deallocate(pSubObject);
          return res;
        }
        else
        {
          W_REPORT_FAILURE("Non-allocatable property should not be part of an object chain!");
        }
      }
      break;
      case WPropertyCategory::Set:
      default:
      {
        W_REPORT_FAILURE("Property of type Set should not be part of an object chain!");
      }
      break;
    }
    return W_FAILURE;
  }
}



W_STATICLINK_FILE(Foundation, Foundation_Reflection_Implementation_PropertyPath);
